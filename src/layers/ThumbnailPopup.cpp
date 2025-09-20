#include <Geode/Geode.hpp>

using namespace geode::prelude;

#include <Geode/modify/LevelInfoLayer.hpp>
#include <Geode/utils/web.hpp>
#include <argon/argon.hpp>
#include "ThumbnailPopup.hpp"

void ThumbnailPopup::onDownload(CCObject* sender){
    CCApplication::sharedApplication()->openURL(ThumbnailManager::get().getThumbnailUrl(m_levelID).c_str());
}
void ThumbnailPopup::onOpenFolder(CCObject* sender){
    geode::utils::file::openFolder(Mod::get()->getSaveDir());
    geode::utils::clipboard::write(fmt::format("{}",this->m_levelID));
    geode::Notification::create("Copied ID to clipboard.",nullptr)->show();
}
void ThumbnailPopup::openDiscordServerPopup(CCObject* sender){
    createQuickPopup(
        "No thumbnail!",
        "This level seems to not have a <cj>Thumbnail</c>...\n"
        "Don't worry, you can submit a thumbnail yourself! Open the level and click the thumbnail button in the pause menu.",
        "No Thanks", "JOIN!",
        [this](auto, bool btn2) {
            if (btn2) {
                CCApplication::sharedApplication()->openURL("https://discord.gg/GuagJDsqds");
            }
        }
    );
}

bool ThumbnailPopup::setup(int id) {
    m_noElasticity = false;
    auto winSize = CCDirector::sharedDirector()->getWinSize();
    setID("ThumbnailPopup");
    setTitle("");

    CCScale9Sprite* border = CCScale9Sprite::create("GJ_square07.png");
    border->setContentSize(m_bgSprite->getContentSize());
    border->setPosition(m_bgSprite->getPosition());
    border->setZOrder(2);

    CCLayerColor* mask = CCLayerColor::create({255, 255, 255});
    mask->setContentSize({391, 220});
    mask->setPosition({m_bgSprite->getContentSize().width / 2 - 391.f/2, m_bgSprite->getContentSize().height/2 - 220.f/2});

    m_bgSprite->setColor({50,50,50});

    m_clippingNode = CCClippingNode::create();
    m_clippingNode->setContentSize(m_bgSprite->getContentSize());
    m_clippingNode->setStencil(mask);
    m_clippingNode->setZOrder(1);

    m_mainLayer->addChild(border);
    m_mainLayer->addChild(m_clippingNode);

    CCSprite* downloadSprite = CCSprite::createWithSpriteFrameName("GJ_downloadBtn_001.png");
    m_downloadBtn = CCMenuItemSpriteExtra::create(downloadSprite, this, menu_selector(ThumbnailPopup::onDownload));
    m_downloadBtn->setEnabled(true);
    m_downloadBtn->setVisible((m_isScreenshotPreview ? false : true));
    m_downloadBtn->setColor({125,125,125});
   
    m_downloadBtn->setPosition({m_mainLayer->getContentSize().width - 5, 5});

    m_buttonMenu->addChild(m_downloadBtn);

    CCSprite* recenterSprite = CCSprite::createWithSpriteFrameName("GJ_undoBtn_001.png");
    CCMenuItemSpriteExtra* recenterBtn = CCMenuItemSpriteExtra::create(recenterSprite, this, menu_selector(ThumbnailPopup::recenter));

    recenterBtn->setPosition({5, 5});
    m_buttonMenu->addChild(recenterBtn);

    #ifdef GEODE_IS_MACOS
    recenterBtn->setVisible(false);
    #endif

    ButtonSprite* infoSprite = ButtonSprite::create((m_isScreenshotPreview ? "Submit" : "What's this?"));
    m_infoBtn = CCMenuItemSpriteExtra::create(infoSprite, this, menu_selector(ThumbnailPopup::openDiscordServerPopup));

    m_infoBtn->setPosition({(m_isScreenshotPreview ? 293.f : m_mainLayer->getContentSize().width/2.f), 6});
    m_infoBtn->setVisible((m_isScreenshotPreview ? true : false));
    m_infoBtn->setZOrder(3);
    m_buttonMenu->addChild(m_infoBtn);

    m_theFunny = CCLabelBMFont::create("OwO", "bigFont.fnt");
    m_theFunny->setPosition(m_bgSprite->getPosition());
    m_theFunny->setVisible((m_isScreenshotPreview ? true : false));
    m_theFunny->setScale(0.25f);

    m_mainLayer->addChild(m_theFunny);

    m_loadingCircle->setParentLayer(m_mainLayer);
    m_loadingCircle->setPosition({ -70,-40 });
    m_loadingCircle->setScale(1.f);
    m_loadingCircle->show();

    this->setTouchEnabled(true);
    cocos2d::CCTouchDispatcher::get()->addTargetedDelegate(this, cocos2d::kCCMenuHandlerPriority, true);
    handleTouchPriority(this);

    m_downloadListener.bind(this, &ThumbnailPopup::handleDownloading);
    m_downloadListener.setFilter(ThumbnailManager::get().fetchThumbnail(
        m_levelID, ThumbnailManager::Quality::High
    ));

    return true;
}

void ThumbnailPopup::recenter(CCObject* sender){

    if(CCNode* node = m_clippingNode->getChildByID("thumbnail")) {
        node->setPosition({m_mainLayer->getContentWidth()/2,m_mainLayer->getContentHeight()/2});
        node->stopAllActions();
        float scale = m_maxHeight/node->getContentSize().height;
        node->setUserObject("new-scale", CCFloat::create(scale));
        node->setScale(scale);
        node->setAnchorPoint({0.5,0.5});
    }
}

void ThumbnailPopup::handleDownloading(ThumbnailManager::FetchTask::Event* event) {
        if (auto res = event->getValue()) {
            if (res->isOk()) {
                this->onDownloadSuccess(res->unwrap());
            } else {
                this->onDownloadError(res->unwrapErr());
            }
        } else if (auto progress = event->getProgress()) {
            //this->updateProgressLabel(progress->downloadProgress().value_or(0.f));
        }
    }

void ThumbnailPopup::onDownloadSuccess(Ref<CCTexture2D> const& texture) {
    // thanks for fucking this up sheepdotcom
    m_downloadBtn->setEnabled(true);
    m_downloadBtn->setColor({255,255,255});

    auto image = CCSprite::createWithTexture(texture);

    float scale = m_maxHeight/image->getContentSize().height;
    image->setScale(scale);
    image->setUserObject("scale", CCFloat::create(scale));
    image->setPosition({m_mainLayer->getContentWidth()/2,m_mainLayer->getContentHeight()/2});

    image->setID("thumbnail");
    m_clippingNode->addChild(image);
    m_loadingCircle->fadeAndRemove();
}

void ThumbnailPopup::onDownloadError(std::string const& error) {
    // thanks for the image cvolton ;)
    CCSprite* image = CCSprite::create("noThumb.png"_spr);
    float scale = m_maxHeight/image->getContentSize().height;
    image->setScale(scale);
    image->setUserObject("scale", CCFloat::create(scale));
    image->setPosition({m_mainLayer->getContentWidth()/2, m_mainLayer->getContentHeight()/2});
    image->setID("thumbnail");

    m_infoBtn->setVisible(true);
    m_theFunny->setVisible(true);
    m_clippingNode->addChild(image);
    m_loadingCircle->fadeAndRemove();

}

ThumbnailPopup* ThumbnailPopup::create(int id,bool screenshotPreview) {
    auto ret = new ThumbnailPopup();
    ret->m_isScreenshotPreview = false;
    ret->m_levelID = id;
    if (ret && ret->initAnchored(395, 225, -1, "GJ_square05.png")) {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}
ThumbnailPopup* ThumbnailPopup::create(int id,CCSprite* image) {
    auto ret = new ThumbnailPopup();
    ret->m_screenshotPreview = image;
    ret->m_isScreenshotPreview = true;
    ret->m_levelID = id;
    if (ret && ret->initAnchored(395, 225, -1, "GJ_square05.png")) {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

float clip(float n, float lower, float upper) {
  return std::max(lower, std::min(n, upper));
}

bool ThumbnailPopup::ccTouchBegan(CCTouch* pTouch, CCEvent* event){
    if (m_touches.size()==1){
        //geode::log::info("this is where the second touch gets added");
        //thank you matcool
        auto firstTouch = *m_touches.begin();

        auto firstLoc = firstTouch->getLocation();
        auto secondLoc = pTouch->getLocation();

        this->m_touchMidPoint = (firstLoc + secondLoc) / 2.f;
        // save current zoom level
        this->m_initialScale = this->getChildByIDRecursive("thumbnail")->getScale();
        // distance between the two touches
        this->m_initialDistance = firstLoc.getDistance(secondLoc);
        // anchor point
        auto thumbnail = this->getChildByIDRecursive("thumbnail");
        auto oldAnchor = thumbnail->getAnchorPoint();
        auto worldPos = thumbnail->convertToWorldSpace({0,0});
        auto newAnchorX = (m_touchMidPoint.x-worldPos.x)/thumbnail->getScaledContentWidth();
        auto newAnchorY = (m_touchMidPoint.y-worldPos.y)/thumbnail->getScaledContentHeight();
        thumbnail->setAnchorPoint({clip(newAnchorX,0,1),clip(newAnchorY,0,1)});
        thumbnail->setPosition({
            thumbnail->getPositionX()+thumbnail->getScaledContentWidth()*-(oldAnchor.x-clip(newAnchorX,0,1)),
            thumbnail->getPositionY()+thumbnail->getScaledContentHeight()*-(oldAnchor.y-clip(newAnchorY,0,1))
        });
    }
    //geode::log::info("touch added");
    m_touches.insert(pTouch);
    return true;
}

void ThumbnailPopup::ccTouchMoved(CCTouch* pTouch, CCEvent* event){
    //geode::log::info("moved");
    if (m_touches.size() == 1){
        //geode::log::info("single touch");
        CCNode* thumbnail = this->getChildByIDRecursive("thumbnail");
        if(!thumbnail) return;
        thumbnail->setPosition(thumbnail->getPositionX()+pTouch->getDelta().x,thumbnail->getPositionY()+pTouch->getDelta().y);
    }
    if (m_touches.size() == 2){
        this->wasZooming = true;
        //geode::log::info("double touch (EPIC!)");
        CCNode* thumbnail = this->getChildByIDRecursive("thumbnail");
        if(!thumbnail) return;
        //thank you matcool
        auto it = m_touches.begin();
        auto firstTouch = *it;
        ++it;
        auto secondTouch = *it;

        auto firstLoc = firstTouch->getLocation();
        auto secondLoc = secondTouch->getLocation();
        auto center = (firstLoc + secondLoc) / 2;
        auto distNow = firstLoc.getDistance(secondLoc);

        auto const mult = this->m_initialDistance / distNow;
        auto zoom = clip(this->m_initialScale / mult, 0.2f,6.5f);
        thumbnail->setScale(zoom);
        //geode::log::info("zoom {}",zoom);

        auto centerDiff = this->m_touchMidPoint - center;
        thumbnail->setPosition(thumbnail->getPosition() - centerDiff);
        this->m_touchMidPoint = center;
    }
}

void ThumbnailPopup::ccTouchEnded(CCTouch* pTouch, CCEvent* event){
    m_touches.erase(pTouch);
    if(wasZooming&&m_touches.size()==1){
        auto thumbnail = this->getChildByIDRecursive("thumbnail");
        auto scale = thumbnail->getScale();
        if (scale<0.25f){
            thumbnail->runAction(
                CCEaseSineInOut::create(
                    CCScaleTo::create(0.5f,0.25f)
                )
            );
        }
        if (scale>4.0f){
            thumbnail->runAction(
                CCEaseSineInOut::create(
                    CCScaleTo::create(0.5f,4.0f)
                )
            );
        }
        wasZooming=false;
    }
    //geode::log::info("ended");
}