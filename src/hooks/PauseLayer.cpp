#include <Geode/modify/PauseLayer.hpp>
#include "../layers/ThumbnailPopup.hpp"
#include "../managers/SettingsManager.hpp"
#include "../utils/RenderTexture.hpp"

#include <prevter.imageplus/include/api.hpp>

using namespace geode::prelude;

struct HideNode {
    CCNode* node = nullptr;
    bool wasVisible = false;

    HideNode(CCNode* parent, std::string_view id) {
        if (parent) {
            node = parent->getChildByID(id);
            if (node) {
                wasVisible = node->isVisible();
                node->setVisible(false);
            }
        }
    }

    ~HideNode() {
        if (node) {
            node->setVisible(wasVisible);
        }
    }
};

#define HIDE_NODE(parent, id) HideNode GEODE_CONCAT(hide_, __COUNTER__)(parent, id);

struct UIObjectState {
    GameObject* object;
    CCPoint oldStartPos;
    CCPoint originalStartPos;
    CCPoint position;
};

static std::vector<UIObjectState> fixUIObjects(PlayLayer* pl) {
    std::vector<UIObjectState> uiObjects;

    for (auto obj : geode::cocos::CCArrayExt<GameObject*>(pl->m_objects)) {
        auto it = pl->m_uiObjectPositions.find(obj->m_uniqueID);
        if (it == pl->m_uiObjectPositions.end()) continue;
        uiObjects.push_back({obj, obj->m_startPosition, it->second, obj->getPosition()});
        obj->setStartPos(it->second);
    }

    pl->positionUIObjects();

    return uiObjects;
}

static void revertUIObjects(PlayLayer* pl, std::vector<UIObjectState> const& uiObjects) {
    for (auto const& state : uiObjects) {
        state.object->setStartPos(state.oldStartPos);
        state.object->setPosition(state.position);
    }

    pl->positionUIObjects();
}

class $modify(ThumbnailPauseLayer, PauseLayer) {
    void customSetup() {
        PauseLayer::customSetup();

        if (!Settings::thumbnailTakingEnabled()) {
            return;
        }

        auto playLayer = PlayLayer::get();
        if (!playLayer || !playLayer->m_level || playLayer->m_level->m_levelID <= 0) {
            return;
        }

        auto rightButtonMenu = this->getChildByID("right-button-menu");
        if (!rightButtonMenu) {
            return;
        }

        auto screenshotSprite = CCSprite::create("thumbnailButton.png"_spr);
        screenshotSprite->setScale(0.6f);
        auto screenshotButton = CCMenuItemSpriteExtra::create(screenshotSprite, this, menu_selector(ThumbnailPauseLayer::onScreenshot));
        rightButtonMenu->addChild(screenshotButton);
        rightButtonMenu->updateLayout();
    }

    void onScreenshot(CCObject*) {
        auto playLayer = PlayLayer::get();
        if (!playLayer) {
            return;
        }

        HIDE_NODE(playLayer, "mat.run-info/RunInfoWidget");
        HIDE_NODE(playLayer, "cheeseworks.speedruntimer/timer");
        HIDE_NODE(playLayer, "sawblade.dim_mode/opacityLabel");
        HIDE_NODE(playLayer, "zilko.xdbot/state-label");
        HIDE_NODE(playLayer, "zilko.xdbot/frame-label");
        HIDE_NODE(playLayer, "zilko.xdbot/recording-audio-label");
        HIDE_NODE(playLayer, "zilko.xdbot/button-menu");
        HIDE_NODE(playLayer, "dankmeme.globed2/game-overlay");
        HIDE_NODE(playLayer, "tobyadd.gdh/labels_top_left");
        HIDE_NODE(playLayer, "tobyadd.gdh/labels_top_right");
        HIDE_NODE(playLayer, "tobyadd.gdh/labels_bottom_left");
        HIDE_NODE(playLayer, "tobyadd.gdh/labels_bottom_right");
        HIDE_NODE(playLayer, "tobyadd.gdh/labels_bottom");
        HIDE_NODE(playLayer, "tobyadd.gdh/labels_top");

        auto oldScale = playLayer->getScaleY();
        playLayer->setScaleY(-oldScale); // flip y-axis because opengl
        auto uiLayerVisible = playLayer->m_uiLayer->isVisible();
        playLayer->m_uiLayer->setVisible(false);
        auto percentage = playLayer->m_percentageLabel->isVisible();
        playLayer->m_percentageLabel->setVisible(false);
        auto progressBarVisible = playLayer->m_progressBar->isVisible();
        playLayer->m_progressBar->setVisible(false);

        auto shader = playLayer->m_shaderLayer;
        bool hadShader = shader->getParent() != nullptr;
        CCSize oldScreenSize{};

        std::vector<UIObjectState> uiObjects;
        std::unique_ptr<uint8_t[]> data;
        bool pixelateHardEdges = false;
        {
            auto oldWinSize = CCDirector::get()->getWinSize();

            RenderTexture rt(1920, 1080);
            rt.begin();

            auto newWinSize = CCDirector::get()->getWinSize();

            // TODO: maybe don't do this if aspect ratio is the same?

            playLayer->m_calculateTargetHeightOffset = true;
            playLayer->m_updateGroundShadows = true;

            playLayer->updateCamera(0.f);
            uiObjects = fixUIObjects(playLayer);

            if (hadShader) {
                // fix shaderlayer
                pixelateHardEdges = shader->m_state.m_pixelateHardEdges;
                oldScreenSize = shader->m_screenSize;
                shader->m_screenSize = newWinSize;
                shader->setupShader(false);
                if (!pixelateHardEdges) {
                    ccTexParams a = {GL_LINEAR,GL_LINEAR};
                    shader->m_sprite->getTexture()->setTexParameters(&a);
                }
                shader->prePixelateShader();
                playLayer->updateShaderLayer(0.f);
            }

            playLayer->updateVisibility(0.f);
            playLayer->visit();

            // mark as dirty again
            playLayer->m_updateGroundShadows = true;
            playLayer->m_calculateTargetHeightOffset = true;

            data = rt.getData();
            rt.end();
        }

        playLayer->setScaleY(oldScale);
        playLayer->m_uiLayer->setVisible(uiLayerVisible);
        playLayer->m_percentageLabel->setVisible(percentage);
        playLayer->m_progressBar->setVisible(progressBarVisible);
        revertUIObjects(playLayer, uiObjects);
        if (hadShader) {
            shader->m_screenSize = oldScreenSize;
            shader->setupShader(false);
            if (!pixelateHardEdges) {
                ccTexParams a = {GL_LINEAR, GL_LINEAR};
                shader->m_sprite->getTexture()->setTexParameters(&a);
            }
            shader->prePixelateShader();
        }

        if (!data) {
            log::error("Failed to take screenshot: could not get pixel data");
            return;
        }

        auto res = imgp::encode::webp(data.get(), 1920, 1080, false, 100);
        if (!res) {
            log::error("Failed to take screenshot: {}", res.unwrapErr());
            return;
        }

        // TODO: upload to server
        auto levelID = PlayLayer::get()->m_level->m_levelID;
        auto saveDir = fmt::format("{}/{}.webp", Mod::get()->getSaveDir(),levelID);
        if (auto saveRes = file::writeBinary(saveDir, *res); !saveRes) {
            log::error("Failed to save screenshot: {}", saveRes.unwrapErr());
            return;
        }

        ThumbnailPopup::create(levelID,saveDir)->show();
    }
};