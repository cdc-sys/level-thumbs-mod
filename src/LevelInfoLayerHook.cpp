#include <Geode/modify/LevelInfoLayer.hpp>
#include "Geode/cocos/CCDirector.h"
#include "Geode/cocos/platform/win32/CCGL.h"
#include "utils.hpp"
#include "ImageCache.hpp"

#include <Geode/Geode.hpp>
using namespace geode::prelude;

class $modify(MyLevelInfoLayer,LevelInfoLayer){
    bool init(GJGameLevel* level, bool challenge) {
        if (!LevelInfoLayer::init(level, challenge)) return false;
        if (!Mod::get()->getSettingValue<bool>("level-bg")) return true;
        auto director = CCDirector::get();
        auto winSize = director->getWinSize();
        auto darkening = static_cast<GLubyte>(255-Mod::get()->getSettingValue<int64_t>("darkening"));

        if(CCImage* image = ImageCache::get()->getImage(fmt::format("thumb-{}", (int)m_level->m_levelID), levelthumbs::getBaseUrl())){
            CCTexture2D* texture = new CCTexture2D();
            texture->initWithImage(image);

            auto bg = this->getChildByID("background");
            bg->setVisible(false);

            auto sprite = CCSprite::createWithTexture(texture);
            sprite->setZOrder(bg->getZOrder());
            sprite->setPosition({winSize.width/2,winSize.height/2});
            sprite->setScale(winSize.width/sprite->getContentWidth());
            sprite->setColor({darkening,darkening,darkening});
            this->addChild(sprite);

            if (Mod::get()->getSettingValue<bool>("enable-blur")){
                // this will do for the minor release, while i search for a good blur shader
                auto alert = FLAlertLayer::create(nullptr,"Coming Soon","<cj>Blur</c> currently isn't implemented, but will be in a <cg>future update</c>.\nStay tuned.\n","OK",nullptr);
                alert->m_scene = this;
                alert->show();
                Mod::get()->setSettingValue<bool>("enable-blur", false);
            }
            

            texture->release();
        }
        return true;
    }
};