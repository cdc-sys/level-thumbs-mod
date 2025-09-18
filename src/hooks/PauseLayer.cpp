#include <Geode/modify/PauseLayer.hpp>
#include "../managers/SettingsManager.hpp"
#include "../utils/RenderTexture.hpp"

#include <prevter.imageplus/include/api.hpp>

using namespace geode::prelude;

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
        // obj->m_startPosition = it->second;
    }

    pl->positionUIObjects();

    return uiObjects;
}

static void revertUIObjects(PlayLayer* pl, std::vector<UIObjectState> const& uiObjects) {
    for (auto const& state : uiObjects) {
        // state.object->m_startPosition = state.oldStartPos;
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

        auto oldScale = playLayer->getScaleY();
        playLayer->setScaleY(-oldScale); // flip y-axis because opengl
        auto uiLayerVisible = playLayer->m_uiLayer->isVisible();
        playLayer->m_uiLayer->setVisible(false);

        auto shader = playLayer->m_shaderLayer;
        bool hadShader = shader->getParent() != nullptr;
        CCSize oldScreenSize{};

        std::vector<UIObjectState> uiObjects;
        std::unique_ptr<uint8_t[]> data;
        {
            RenderTexture rt(1920, 1080);
            rt.begin();

            auto newWinSize = CCDirector::get()->getWinSize();

            // TODO: maybe don't do this if aspect ratio is the same?

            playLayer->m_calculateTargetHeightOffset = true;
            playLayer->m_updateGroundShadows = true;

            playLayer->updateCamera(0.f);
            uiObjects = fixUIObjects(playLayer);

            if (hadShader) {
                // fix the shader layer
                oldScreenSize = shader->m_screenSize;
                shader->m_screenSize = newWinSize;
                shader->setupShader(false);
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
        revertUIObjects(playLayer, uiObjects);
        if (hadShader) {
            shader->m_screenSize = oldScreenSize;
            shader->setupShader(false);
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
        file::writeBinary("TestThumbnail.webp", *res);
    }
};