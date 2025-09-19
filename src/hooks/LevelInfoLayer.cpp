#include <Geode/modify/LevelInfoLayer.hpp>
#include "../managers/SettingsManager.hpp"
#include "../managers/ThumbnailManager.hpp"
#include "../layers/ThumbnailPopup.hpp"

using namespace geode::prelude;

constexpr auto vertexShader = R"(
attribute vec4 a_position;
attribute vec4 a_color;
attribute vec2 a_texCoord;

#ifdef GL_ES
varying lowp vec4 v_fragmentColor;
varying mediump vec2 v_texCoord;
#else
varying vec4 v_fragmentColor;
varying vec2 v_texCoord;
#endif

void main() {
    gl_Position = CC_MVPMatrix * a_position;
    v_fragmentColor = a_color;
    v_texCoord = a_texCoord;
})";

constexpr auto fragmentShaderHorizontal = R"(
#ifdef GL_ES
precision mediump float;
#endif

varying vec4 v_fragmentColor;
varying vec2 v_texCoord;

uniform sampler2D u_texture;
uniform vec2 u_screenSize;
uniform float u_radius;

void main() {
    float scaledRadius = u_radius * u_screenSize.y * 0.5;
    vec2 texOffset = 1.0 / u_screenSize;
    vec2 direction = vec2(texOffset.x, 0.0); // horizontal blur

    vec3 result = texture2D(u_texture, v_texCoord).rgb;
    float weightSum = 1.0;
    float weight = 1.0;

    // Use fast blur method
    float fastScale = u_radius * 10.0 / ((u_radius * 10.0 + 1.0) * (u_radius * 10.0 + 1.0) - 1.0);
    scaledRadius *= fastScale;

    for (int i = 1; i < 64; i++) {
        if (float(i) >= scaledRadius) break;

        weight -= 1.0 / scaledRadius;
        if (weight <= 0.0) break;

        vec2 offset = direction * float(i);
        result += texture2D(u_texture, v_texCoord + offset).rgb * weight;
        result += texture2D(u_texture, v_texCoord - offset).rgb * weight;
        weightSum += 2.0 * weight;
    }

    result /= weightSum;
    gl_FragColor = vec4(result, 1.0) * v_fragmentColor;
})";

constexpr auto fragmentShaderVertical = R"(
#ifdef GL_ES
precision mediump float;
#endif

varying vec4 v_fragmentColor;
varying vec2 v_texCoord;

uniform sampler2D u_texture;
uniform vec2 u_screenSize;
uniform float u_radius;

void main() {
    float scaledRadius = u_radius * u_screenSize.y * 0.5;
    vec2 texOffset = 1.0 / u_screenSize;
    vec2 direction = vec2(0.0, texOffset.y); // vertical blur

    vec3 result = texture2D(u_texture, v_texCoord).rgb;
    float weightSum = 1.0;
    float weight = 1.0;

    // Use fast blur method
    float fastScale = u_radius * 10.0 / ((u_radius * 10.0 + 1.0) * (u_radius * 10.0 + 1.0) - 1.0);
    scaledRadius *= fastScale;

    for (int i = 1; i < 64; i++) {
        if (float(i) >= scaledRadius) break;

        weight -= 1.0 / scaledRadius;
        if (weight <= 0.0) break;

        vec2 offset = direction * float(i);
        result += texture2D(u_texture, v_texCoord + offset).rgb * weight;
        result += texture2D(u_texture, v_texCoord - offset).rgb * weight;
        weightSum += 2.0 * weight;
    }

    result /= weightSum;
    gl_FragColor = vec4(result, 1.0) * v_fragmentColor;
})";

static CCGLProgram* getOrCreateShader(char const* key, char const* vertexSrc, char const* fragmentSrc) {
    auto shaderCache = CCShaderCache::sharedShaderCache();
    if (auto program = shaderCache->programForKey(key)) {
        return program;
    }

    auto program = new CCGLProgram();
    program->initWithVertexShaderByteArray(vertexSrc, fragmentSrc);
    program->addAttribute("a_position", kCCVertexAttrib_Position);
    program->addAttribute("a_color", kCCVertexAttrib_Color);
    program->addAttribute("a_texCoord", kCCVertexAttrib_TexCoords);

    if (!program->link()) {
        log::error("Failed to link shader: {}", key);
        program->release();
        return nullptr;
    }

    program->updateUniforms();
    shaderCache->addProgram(program, key);
    program->release();
    return program;
}

static void applyBlurPass(CCSprite* input, CCRenderTexture* output, CCGLProgram* program, CCSize const& size, float radius) {
    input->setShaderProgram(program);
    input->setPosition(size * 0.5f);

    program->use();
    program->setUniformsForBuiltins();
    program->setUniformLocationWith2f(
        program->getUniformLocationForName("u_screenSize"),
        size.width, size.height
    );
    program->setUniformLocationWith1f(
        program->getUniformLocationForName("u_radius"),
        radius
    );

    output->begin();
    input->visit();
    output->end();
}

class $modify(ThumbnailLevelInfoLayer, LevelInfoLayer) {
    struct Fields {
        EventListener<ThumbnailManager::FetchTask> m_fetchListener;
        CCSprite* m_background = nullptr;
    };

    void onDownloadSuccess(CCTexture2D* texture) {
        auto fields = m_fields.self();

        if (auto old = fields->m_background) {
            old->removeFromParent();
        }

        auto bg = this->getChildByID("background");
        if (bg) {
            bg->setVisible(false);
        }

        auto winSize = CCDirector::get()->getWinSize();

        auto sprite = CCSprite::createWithTexture(texture);
        sprite->setID("level-bg"_spr);
        sprite->setPosition(winSize * 0.5f);
        sprite->setScale(std::max(
            winSize.width / sprite->getContentWidth(),
            winSize.height / sprite->getContentHeight()
        ));

        // Improve texture filtering to reduce pixelation when scaling
        ccTexParams params{GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE};
        texture->setTexParameters(&params);

        uint8_t dim = 255 - Settings::backgroundDimAmount();
        sprite->setColor({dim, dim, dim});

        if (Settings::isBackgroundBlurred()) {
            auto rtA = CCRenderTexture::create(winSize.width, winSize.height);
            auto rtB = CCRenderTexture::create(winSize.width, winSize.height);

            auto blurH = getOrCreateShader("blur-horizontal"_spr, vertexShader, fragmentShaderHorizontal);
            auto blurV = getOrCreateShader("blur-vertical"_spr, vertexShader, fragmentShaderVertical);
            if (!blurH || !blurV) {
                this->addChild(sprite, bg ? bg->getZOrder() : -1);
                fields->m_background = sprite;
                return;
            }

            float radius = static_cast<float>(Settings::backgroundBlurRadius()) * 0.03f;

            // horizontal pass
            applyBlurPass(sprite, rtA, blurH, winSize, radius);

            // vertical pass
            applyBlurPass(rtA->getSprite(), rtB, blurV, winSize, radius);

            // add final result
            auto finalSprite = rtB->getSprite();
            finalSprite->setPosition(winSize * 0.5f);
            finalSprite->setID("level-bg"_spr);

            this->addChild(finalSprite, bg ? bg->getZOrder() : -1);
            fields->m_background = finalSprite;
        } else {
            this->addChild(sprite, bg ? bg->getZOrder() : -1);
            fields->m_background = sprite;
        }
    }

    void handleDownloading(ThumbnailManager::FetchTask::Event* event) {
        if (auto res = event->getValue()) {
            if (res->isOk()) {
                this->onDownloadSuccess(res->unwrap());
            }
        }
    }

    void updateBackground(int32_t levelID, ThumbnailManager::Quality quality) {
        auto fields = m_fields.self();

        fields->m_fetchListener.bind(this, &ThumbnailLevelInfoLayer::handleDownloading);
        fields->m_fetchListener.setFilter(ThumbnailManager::get().fetchThumbnail(levelID, quality));
    }

    $override bool init(GJGameLevel* level, bool challenge) {
        if (!LevelInfoLayer::init(level, challenge)) {
            return false;
        }

        if (auto menu = getChildByID("left-side-menu")) {
            if (Settings::showThumbnailButton()) {
                auto sprite = CCSprite::create("thumbnailButton.png"_spr);
                auto button = CCMenuItemExt::createSpriteExtra(sprite,[this](CCObject* sender){
                    ThumbnailPopup::create(m_level->m_levelID)->show();
                });
        
                button->setID("thumbnail-button");
                menu->addChild(button);
                menu->updateLayout();
            }
        }

        if (!Settings::isShowLevelBackground()) {
            return true;
        }

        auto bgQuality = Settings::backgroundQuality();
        auto levelID = level->m_levelID.value();

        bool fetchBigger = !Settings::isBackgroundBlurred() && bgQuality != ThumbnailManager::Quality::Small;
        bool hasSmall = ThumbnailManager::get().getCacheState(levelID, ThumbnailManager::Quality::Small) != ThumbnailManager::CacheState::NotCached;

        // check if we already have a small thumbnail to show
        if (hasSmall) {
            // check if the desired quality is also cached to avoid flickering
            if (fetchBigger && ThumbnailManager::get().getCacheState(levelID, bgQuality) != ThumbnailManager::CacheState::NotCached) {
                this->updateBackground(levelID, bgQuality);
                return true;
            }

            // start with the low quality thumbnail
            this->updateBackground(levelID, ThumbnailManager::Quality::Small);
        }

        // if we only want a small thumbnail, and we have it, we're done
        if (!fetchBigger && hasSmall) {
            return true;
        }

        // if we don't have shaders enabled, and the quality is not small, run a fetch for the new quality
        this->updateBackground(levelID, bgQuality);


        return true;
    }
};