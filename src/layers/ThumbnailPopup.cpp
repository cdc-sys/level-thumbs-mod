#include "ConfirmAlertLayer.hpp"
#include "GUI/CCControlExtension/CCScale9Sprite.h"
#include "Geode/cocos/textures/CCTextureCache.h"
#include "Geode/ui/LoadingSpinner.hpp"
#include "Geode/ui/Popup.hpp"
#include "Geode/ui/TextArea.hpp"
#include "LoadingOverlay.hpp"
#include <Geode/Geode.hpp>
#include <fmt/format.h>
#include <chrono>

using namespace geode::prelude;

#include <Geode/utils/web.hpp>
#include <argon/argon.hpp>
#include "ThumbnailPopup.hpp"

void ThumbnailPopup::onDownload(CCObject* sender) {
    CCApplication::sharedApplication()->openURL(ThumbnailManager::getThumbnailUrl(m_levelID).c_str());
}

void ThumbnailPopup::onOpenFolder(CCObject* sender) {
    file::openFolder(Mod::get()->getSaveDir());
    clipboard::write(fmt::to_string(this->m_levelID));
    Notification::create("Copied ID to clipboard.", nullptr)->show();
}

void ThumbnailPopup::openDiscordServerPopup(CCObject* sender) {
    if (m_isPreview){
        createQuickPopup(
            "Confirmation",
            "Are you sure you want to submit?",
            "No", "Yes",
            [this](auto, bool btn2){
                if (!Mod::get()->getSavedValue<bool>("showed-rules") && btn2){
                    ConfirmAlertLayer::createRulesPopup(
                        [this](bool btn2){
                            if (btn2) {
                                runSubmissionLogic();
                                Mod::get()->setSavedValue<bool>("showed-rules", true);
                            }
                        },
                        "Submission Rules",
                        "submission_rules.md",
                        "OK","Submit"
                    )->show();
                } else {
                    if (btn2) runSubmissionLogic();
                }
            }
        );
    } else {
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
}
void ThumbnailPopup::runSubmissionLogic() {
    m_uploadListener.bind(this, &ThumbnailPopup::handleUploading);
    m_uploadListener.setFilter(AuthManager::get().uploadThumbnail(m_previewFileName, m_levelID, true));
}

void ThumbnailPopup::handleUploading(AuthManager::UploadTask::Event* event) {
    if (auto res = event->getValue()) {
        if (res->isOk()) {
            FLAlertLayer::create("Success!",res->unwrapOrDefault(),"OK")->show();
        } else {
            FLAlertLayer::create("Error!",fmt::format("<cr>{}</c>", res->err().value_or("Unknown error")), "OK")->show();
        }
    } else if (auto progress = event->getProgress()) {
        //this->updateProgressLabel(progress->downloadProgress().value_or(0.f));
    }
}

bool ThumbnailPopup::setup(int id) {
    m_noElasticity = false;
    auto winSize = CCDirector::sharedDirector()->getWinSize();
    this->setID("ThumbnailPopup");
    this->setTitle("");

    CCScale9Sprite* border = CCScale9Sprite::create("GJ_square07.png");
    border->setContentSize(m_bgSprite->getContentSize());
    border->setPosition(m_bgSprite->getPosition());
    border->setZOrder(2);

    CCLayerColor* mask = CCLayerColor::create({255, 255, 255});
    mask->setContentSize({391, 220});
    mask->setPosition({m_bgSprite->getContentSize().width/2 - 391.f/2, m_bgSprite->getContentSize().height/2 - 220.f/2});

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
    m_downloadBtn->setVisible(!m_isPreview);
    m_downloadBtn->setColor({125,125,125});
   
    m_downloadBtn->setPosition({m_mainLayer->getContentSize().width - 5, 5});

    m_buttonMenu->addChild(m_downloadBtn);

    CCSprite* infoBtn = CCSprite::createWithSpriteFrameName("GJ_infoIcon_001.png");
    CCSprite* infoBtnDark = CCSprite::createWithSpriteFrameName("GJ_infoIcon_001.png");
    infoBtnDark->setColor({100,100,100});
    m_thumbInfoBtn = CCMenuItemExt::createToggler(infoBtnDark, infoBtn, [this](auto self){
        this->m_mainLayer->getChildByID("thumbnail-info")->setVisible(!self->isOn());
        Mod::get()->setSavedValue<bool>("show-info", !self->isOn());
    });
    m_thumbInfoBtn->toggle(Mod::get()->getSavedValue<bool>("show-info"));
    m_thumbInfoBtn->setVisible(!m_isPreview);
   
    m_thumbInfoBtn->setPosition({m_mainLayer->getContentSize().width - 5, 220});

    m_buttonMenu->addChild(m_thumbInfoBtn);

    CCSprite* recenterSprite = CCSprite::createWithSpriteFrameName("GJ_undoBtn_001.png");
    CCMenuItemSpriteExtra* recenterBtn = CCMenuItemSpriteExtra::create(recenterSprite, this, menu_selector(ThumbnailPopup::recenter));

    recenterBtn->setPosition({5, 5});
    m_buttonMenu->addChild(recenterBtn);

    #ifdef GEODE_IS_MACOS
    recenterBtn->setVisible(false);
    #endif

    ButtonSprite* infoSprite = ButtonSprite::create(m_isPreview ? "Submit" : "What's this?");
    m_infoBtn = CCMenuItemSpriteExtra::create(infoSprite, this, menu_selector(ThumbnailPopup::openDiscordServerPopup));

    m_infoBtn->setPosition({m_mainLayer->getContentSize().width/2.f, 6});
    m_infoBtn->setVisible(m_isPreview);
    m_infoBtn->setZOrder(3);
    m_buttonMenu->addChild(m_infoBtn);

    m_theFunny = CCLabelBMFont::create(m_isPreview ? "Hiiii\ngeming\npopcorn\nskepper\nbob\nanvixo\nmoonstarmaster\ncdc\nlevel thumbnails bot\norangeyguy\ncrazytoast\nfuzzy\nkirky bonzai\nsilly billy" : "OwO", "bigFont.fnt");
    m_theFunny->setPosition(m_bgSprite->getPosition());
    m_theFunny->setVisible(m_isPreview);
    m_theFunny->setScale(0.25f);

    m_mainLayer->addChild(m_theFunny);

    m_loadingCircle->setParentLayer(m_mainLayer);
    m_loadingCircle->setPosition({m_mainLayer->getContentWidth()/2,m_mainLayer->getContentHeight()/2});
    m_loadingCircle->setAnchorPoint({0.5,0.5});
    m_loadingCircle->setScale(1.f);
    m_loadingCircle->ignoreAnchorPointForPosition(false);
    m_loadingCircle->show();

    this->setTouchEnabled(true);
    CCTouchDispatcher::get()->addTargetedDelegate(this, kCCMenuHandlerPriority, true);
    handleTouchPriority(this);

    if (!m_isPreview){
        m_downloadListener.bind(this, &ThumbnailPopup::handleDownloading);
        m_downloadListener.setFilter(ThumbnailManager::get().fetchThumbnail(
            m_levelID, ThumbnailManager::Quality::High
        ));
        this->loadThumbnailInfo();
    } else {
        CCTextureCache::get()->removeTextureForKey(this->m_previewFileName.c_str());
        this->onDownloadSuccess(Ref<CCTexture2D>(CCSprite::create(this->m_previewFileName.c_str())->getTexture()));
    }

    return true;
}

void ThumbnailPopup::recenter(CCObject* sender) {
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
    image->setPosition({m_mainLayer->getContentWidth()/2, m_mainLayer->getContentHeight()/2});

    image->setID("thumbnail");
    m_clippingNode->addChild(image);
    m_loadingCircle->fadeAndRemove();
}

void ThumbnailPopup::onDownloadError(std::string const& error) {
    // thanks for the image cvolton ;)
    CCSprite* image = CCSprite::create("noThumb.png"_spr);
    float scale = m_maxHeight / image->getContentSize().height;
    image->setScale(scale);
    image->setUserObject("scale", CCFloat::create(scale));
    image->setPosition({m_mainLayer->getContentWidth()/2, m_mainLayer->getContentHeight()/2});
    image->setID("thumbnail");

    m_infoBtn->setVisible(true);
    m_theFunny->setVisible(true);
    m_clippingNode->addChild(image);
    m_loadingCircle->fadeAndRemove();

}

void ThumbnailPopup::loadThumbnailInfo(){
    auto req = web::WebRequest();
    m_infoListener.setFilter(req.get(fmt::format("{}/thumbnail/{}/info", Settings::thumbnailAPIBaseURL(), m_levelID)));
    
    SimpleTextArea* textArea = SimpleTextArea::create("Loading info...");
    textArea->setAnchorPoint({0, 0});
    textArea->setPosition({5, 5});
    textArea->setScale(0.7f);
    textArea->setVisible(false);

    LoadingSpinner* loader = LoadingSpinner::create(45.f/2);
    loader->setAnchorPoint({0, 0});
    loader->setPosition({2.5f, 2.5f});

    CCScale9Sprite* bg = CCScale9Sprite::create("square02b_001.png");
    bg->setColor({0, 0, 0});
    bg->setOpacity(120);
    bg->setScale(0.5f);
    //bg->setContentWidth(textArea->getScaledContentWidth()+10*2);
    //bg->setContentHeight(textArea->getScaledContentHeight()+10*2);
    bg->setContentWidth(55);
    bg->setContentHeight(55);
    bg->setAnchorPoint({0, 0});

    CCNode* container = CCNode::create();
    container->setID("thumbnail-info");
    container->setAnchorPoint({0, 0});
    container->setPosition({10, 10});
    m_buttonMenu->setZOrder(100);
    container->setZOrder(99);

    container->addChild(bg);
    container->addChild(textArea);
    container->addChild(loader);
    
    container->setVisible(Mod::get()->getSavedValue<bool>("show-info"));

    this->m_mainLayer->addChild(container);
    m_infoListener.bind([textArea, container, bg, loader](web::WebTask::Event* e){
        if (auto res = e->getValue()) {
            if (res->ok()){
                auto json = res->json().unwrapOrDefault();
                std::string uploader = json["username"].asString().unwrapOr("Unknown");
                std::string upload_time = json["upload_time"].asString().unwrapOr("Unknown");
                std::string first_upload_time = json["first_upload_time"].asString().unwrapOr("Unknown");
                std::string accepted_time = json["accepted_time"].asString().unwrapOr("Unknown");
                std::string accepter = json["accepted_by_username"].asString().unwrapOr("Unknown");
                
                textArea->setVisible(true);
                textArea->setText(fmt::format(
                    "Submitted by: {}\n"
                    "Submitted at: {}\n"
                    "First submitted at: {}\n"
                    "Accepted by: {}\n"
                    "Accepted at: {}",
                    uploader, upload_time,
                    first_upload_time, accepter,
                    accepted_time
                ));
                bg->setContentWidth((textArea->getScaledContentWidth()+10)*2);
                bg->setContentHeight((textArea->getScaledContentHeight()+10)*2);
                loader->removeFromParent();
            } else{
                container->setVisible(false);
            }
        }
    });
}

ThumbnailPopup* ThumbnailPopup::create(int id, bool screenshotPreview) {
    auto ret = new ThumbnailPopup();
    ret->m_isPreview = false;
    ret->m_levelID = id;
    if (ret->initAnchored(395, 225, -1, "GJ_square05.png")) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

ThumbnailPopup* ThumbnailPopup::create(int id, std::string filename) {
    auto ret = new ThumbnailPopup();
    ret->m_previewFileName = std::move(filename);
    ret->m_isPreview = true;
    ret->m_levelID = id;
    if (ret->initAnchored(395, 225, -1, "GJ_square05.png")) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

float clip(float n, float lower, float upper) {
  return std::max(lower, std::min(n, upper));
}

bool ThumbnailPopup::ccTouchBegan(CCTouch* pTouch, CCEvent* event){
    if (m_touches.size() == 1){
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
        auto worldPos = thumbnail->convertToWorldSpace({0, 0});
        auto newAnchorX = (m_touchMidPoint.x-worldPos.x) / thumbnail->getScaledContentWidth();
        auto newAnchorY = (m_touchMidPoint.y-worldPos.y) / thumbnail->getScaledContentHeight();
        thumbnail->setAnchorPoint({clip(newAnchorX,0,1), clip(newAnchorY,0,1)});
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
        if (!thumbnail) return;
        thumbnail->setPosition(thumbnail->getPosition() + pTouch->getDelta());
    }
    if (m_touches.size() == 2){
        this->wasZooming = true;
        //geode::log::info("double touch (EPIC!)");
        CCNode* thumbnail = this->getChildByIDRecursive("thumbnail");
        if (!thumbnail) return;
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
        auto zoom = clip(this->m_initialScale / mult, 0.2f, 6.5f);
        thumbnail->setScale(zoom);
        //geode::log::info("zoom {}",zoom);

        auto centerDiff = this->m_touchMidPoint - center;
        thumbnail->setPosition(thumbnail->getPosition() - centerDiff);
        this->m_touchMidPoint = center;
    }
}

void ThumbnailPopup::ccTouchEnded(CCTouch* pTouch, CCEvent* event){
    m_touches.erase(pTouch);
    if (wasZooming && m_touches.size() == 1){
        auto thumbnail = this->getChildByIDRecursive("thumbnail");
        auto scale = thumbnail->getScale();
        if (scale < 0.25f){
            thumbnail->runAction(
                CCEaseSineInOut::create(
                    CCScaleTo::create(0.5f, 0.25f)
                )
            );
        }
        if (scale > 4.0f){
            thumbnail->runAction(
                CCEaseSineInOut::create(
                    CCScaleTo::create(0.5f, 4.0f)
                )
            );
        }
        wasZooming = false;
    }
    //geode::log::info("ended");
}