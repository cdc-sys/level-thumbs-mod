#include <Geode/modify/LevelCell.hpp>

#include "../managers/SettingsManager.hpp"
#include "../managers/ThumbnailManager.hpp"

using namespace geode::prelude;

class $modify(ThumbnailLevelCell, LevelCell) {
    struct Fields {
        EventListener<ThumbnailManager::FetchTask> m_fetchListener;
        CCLabelBMFont* m_progressLabel = nullptr;
        LoadingSpinner* m_spinner = nullptr;
        CCClippingNode* m_clippingNode = nullptr;
        CCLayerColor* m_separator = nullptr;
    };

    void updateProgressLabel(float progress) {
        auto fields = m_fields.self();

        // update existing label
        auto text = fmt::format("{:.0f}%", progress);
        if (auto label = fields->m_progressLabel) {
            label->setString(text.c_str());
            return;
        }

        // create label
        auto label = CCLabelBMFont::create(text.c_str(),"bigFont.fnt");
        label->setPosition({352, 1});
        label->setAnchorPoint({1, 0});
        label->setScale(0.25f);
        label->setOpacity(128);
        label->setID("download-progress"_spr);
        fields->m_progressLabel = label;
        this->addChild(label);

        // loading indicator
        auto spinner = LoadingSpinner::create(18.f);
        spinner->setPosition(m_compactView ? ccp(272, 25) : ccp(334, 15));
        spinner->setID("spinner"_spr);
        fields->m_spinner = spinner;
        this->addChild(spinner);
    }

    void removeLoadingIndicators() {
        auto fields = m_fields.self();
        if (auto label = fields->m_progressLabel) {
            label->removeFromParent();
            fields->m_progressLabel = nullptr;
        }
        if (auto spinner = fields->m_spinner) {
            spinner->removeFromParent();
            fields->m_spinner = nullptr;
        }
    }

    void fixDailyCell() {
        constexpr float dailyMult = 1.22;

        auto fields = m_fields.self();

        fields->m_separator->setScaleX(0.45 * dailyMult);
        fields->m_separator->setScaleY(dailyMult);
        fields->m_separator->setPosition({m_backgroundLayer->getContentWidth() - (fields->m_separator->getContentWidth() * dailyMult)/2 - 20 + 7, -7.9f});
        fields->m_clippingNode->setScale(dailyMult);
        fields->m_clippingNode->setPosition(fields->m_clippingNode->getPosition().x + 7, -7.9f);

        auto parent = getParent();
        if (!parent) return;

        if (auto bg = typeinfo_cast<CCScale9Sprite*>(parent->getChildByID("background"))){
            CCScale9Sprite* border = CCScale9Sprite::create("GJ_square07.png");
            border->setContentSize(bg->getContentSize());
            border->setPosition(bg->getPosition());
            border->setColor(bg->getColor());
            border->setZOrder(5);
            border->setID("border"_spr);
            parent->addChild(border);
        }

        if (auto node = parent->getChildByID("crown-sprite")){
            node->setZOrder(6);
        }
    }

    void onDownloadSuccess(Ref<CCTexture2D> const& texture) {
        this->removeLoadingIndicators();

        auto fields = m_fields.self();
        m_backgroundLayer->setZOrder(-2);

        auto sprite = CCSprite::createWithTexture(texture);
        sprite->setID("thumbnail"_spr);
        float imgScale = m_backgroundLayer->getContentHeight() / sprite->getContentHeight();
        sprite->setScale(imgScale);

        float separatorXMul = 1;
        if (m_compactView){
            sprite->setScale(imgScale * 1.3f);
            separatorXMul = 0.75;
        }

        constexpr float angle = 18;

        auto rect = CCLayerColor::create({255, 255, 255});
        CCSize scaledImageSize{sprite->getScaledContentWidth(), sprite->getContentHeight() * imgScale};
        rect->setSkewX(angle);
        rect->setContentSize(scaledImageSize);
        rect->setAnchorPoint({1, 0});
        
        auto clippingNode = CCClippingNode::create();
        clippingNode->setStencil(rect);
        clippingNode->addChild(sprite);
        clippingNode->setContentSize(scaledImageSize);
        clippingNode->setAnchorPoint({1, 0});
        clippingNode->setPosition({m_backgroundLayer->getContentWidth(), 0.3f});
        clippingNode->setID("clipping-node"_spr);
        fields->m_clippingNode = clippingNode;

        float scale =  m_backgroundLayer->getContentHeight() / clippingNode->getContentHeight();
        // rect->setScale(scale);
        sprite->setPosition(clippingNode->getContentSize() * 0.5f);

        auto separator = CCLayerColor::create({0, 0, 0});
        separator->setZOrder(-2);
        separator->setOpacity(50);
        separator->setScaleX(0.45f);
        separator->ignoreAnchorPointForPosition(false);
        separator->setSkewX(angle*2);
        separator->setContentSize(scaledImageSize);
        separator->setAnchorPoint({1, 0});
        separator->setPosition({m_backgroundLayer->getContentWidth() - separator->getContentWidth()/2 - (20 * separatorXMul), 0.3f});
        separator->setID("separator"_spr);
        fields->m_separator = separator;
        this->addChild(separator);

        clippingNode->setZOrder(-1);
        this->addChild(clippingNode);

        auto parent = getParent();
        if (typeinfo_cast<DailyLevelNode*>(parent) || typeinfo_cast<CCScale9Sprite*>(parent)){
            this->fixDailyCell();
        }
    }

    void onDownloadError(std::string const& error) {
        this->removeLoadingIndicators();
    }

    void handleDownloading(ThumbnailManager::FetchTask::Event* event) {
        if (auto res = event->getValue()) {
            if (res->isOk()) {
                this->onDownloadSuccess(res->unwrap());
            } else {
                this->onDownloadError(res->unwrapErr());
            }
        } else if (auto progress = event->getProgress()) {
            this->updateProgressLabel(progress->downloadProgress().value_or(0.f));
        }
    }

    $override void loadCustomLevelCell() {
        LevelCell::loadCustomLevelCell();
        if (!Settings::showInBrowser()) {
            return;
        }

        auto fields = m_fields.self();
        fields->m_fetchListener.bind(this, &ThumbnailLevelCell::handleDownloading);
        fields->m_fetchListener.setFilter(ThumbnailManager::get().fetchThumbnail(
            m_level->m_levelID, ThumbnailManager::Quality::Small
        ));
    }
};