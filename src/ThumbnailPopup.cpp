#include <Geode/Geode.hpp>

using namespace geode::prelude;

#include <Geode/modify/LevelInfoLayer.hpp>
#include <Geode/utils/web.hpp>
#include "ThumbnailPopup.hpp"
#include "utils.hpp"
#include "ImageCache.hpp"

void ThumbnailPopup::onDownload(CCObject* sender){
    std::string URL = fmt::format("https://raw.githubusercontent.com/cdc-sys/level-thumbnails/main/thumbs/{}.png", m_levelID);
    CCApplication::sharedApplication()->openURL(URL.c_str());
}

void ThumbnailPopup::openDiscordServerPopup(CCObject* sender){
    
    createQuickPopup(
        "Uh Oh!",
        "Hm.. This level seems to not have a <cj>Thumbnail</c>...\n"
        "Worry not! You can join our <cg>Discord Server</c> and submit a thumbnail <cy>YOURSELF!</c>",
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
    mask->setPosition({m_bgSprite->getContentSize().width / 2 - 391/2, m_bgSprite->getContentSize().height/2 - 220/2});

    m_bgSprite->setColor({50,50,50});

    m_clippingNode = CCClippingNode::create();
    m_clippingNode->setContentSize(m_bgSprite->getContentSize());
    m_clippingNode->setStencil(mask);
    m_clippingNode->setZOrder(1);

    m_mainLayer->addChild(border);
    m_mainLayer->addChild(m_clippingNode);

    CCSprite* downloadSprite = CCSprite::createWithSpriteFrameName("GJ_downloadBtn_001.png");
    m_downloadBtn = CCMenuItemSpriteExtra::create(downloadSprite, this, menu_selector(ThumbnailPopup::onDownload));
    m_downloadBtn->setEnabled(false);
    m_downloadBtn->setColor({125,125,125});
   
    m_downloadBtn->setPosition({m_mainLayer->getContentSize().width - 5, 5});

    m_buttonMenu->addChild(m_downloadBtn);

    CCSprite* recenterSprite = CCSprite::createWithSpriteFrameName("GJ_undoBtn_001.png");
    CCMenuItemSpriteExtra* recenterBtn = CCMenuItemSpriteExtra::create(recenterSprite, this, menu_selector(ThumbnailPopup::recenter));

    recenterBtn->setPosition({5, 5});
    m_buttonMenu->addChild(recenterBtn);

    #ifndef GEODE_IS_WINDOWS
        recenterBtn->setVisible(false);
    #endif

    ButtonSprite* infoSprite = ButtonSprite::create("What's this?");
    m_infoBtn = CCMenuItemSpriteExtra::create(infoSprite, this, menu_selector(ThumbnailPopup::openDiscordServerPopup));

    m_infoBtn->setPosition({m_mainLayer->getContentSize().width/2, 6});
    m_infoBtn->setVisible(false);
    m_infoBtn->setZOrder(3);

    m_buttonMenu->addChild(m_infoBtn);

    m_theFunny = CCLabelBMFont::create("OwO", "bigFont.fnt");
    m_theFunny->setPosition(m_bgSprite->getPosition());
    m_theFunny->setVisible(false);
    m_theFunny->setScale(0.25f);

    m_mainLayer->addChild(m_theFunny);

    m_loadingCircle->setParentLayer(m_mainLayer);
    m_loadingCircle->setPosition({ -70,-40 });
    m_loadingCircle->setScale(1.f);
    m_loadingCircle->show();

    if(CCImage* image = ImageCache::get()->getImage(fmt::format("thumb-{}", m_levelID))){
        m_image = image;
        m_loadingCircle->fadeAndRemove();
        imageCreationFinished(m_image);
        return true;
    }
    
    std::string URL = fmt::format("https://raw.githubusercontent.com/cdc-sys/level-thumbnails/main/thumbs/{}.png", m_levelID);

    auto req = web::WebRequest();
    m_downloadListener.bind([this](web::WebTask::Event* e){
        if (auto res = e->getValue()){
            if (!res->ok()) {
                onDownloadFail();
            } else {
                auto data = res->data();
                std::thread imageThread = std::thread([data,this](){
                    m_image = new CCImage();
                    m_image->autorelease();
                    m_image->initWithImageData(const_cast<uint8_t*>(data.data()),data.size());
                    geode::Loader::get()->queueInMainThread([this](){
                        ImageCache::get()->addImage(m_image, fmt::format("thumb-{}", m_levelID));
                        imageCreationFinished(m_image);
                    });
                });
                imageThread.detach();
            }
        }
    });
    auto downloadTask = req.get(URL);
    m_downloadListener.setFilter(downloadTask);
    

    return true;
}

void ThumbnailPopup::imageCreationFinished(CCImage* image){
    CCTexture2D* texture = new CCTexture2D();
    texture->autorelease();
    texture->initWithImage(image);
    onDownloadFinished(CCSprite::createWithTexture(texture));
}

void ThumbnailPopup::recenter(CCObject* sender){

    if(CCNode* node = m_clippingNode->getChildByID("thumbnail")) {
        node->setPosition({m_mainLayer->getContentWidth()/2,m_mainLayer->getContentHeight()/2});
        node->stopAllActions();
        float scale = m_maxHeight/node->getContentSize().height;
        node->setUserObject("new-scale", CCFloat::create(scale));
        node->setScale(scale);
    }
}

void ThumbnailPopup::onDownloadFinished(CCSprite* image) {
    // thanks for fucking this up sheepdotcom
    m_downloadBtn->setEnabled(true);
    m_downloadBtn->setColor({255,255,255});

    float scale = m_maxHeight/image->getContentSize().height;
    image->setScale(scale);
    image->setUserObject("scale", CCFloat::create(scale));
    image->setPosition({m_mainLayer->getContentWidth()/2,m_mainLayer->getContentHeight()/2});

    image->setID("thumbnail");
    m_clippingNode->addChild(image);
    m_loadingCircle->fadeAndRemove();
}

void ThumbnailPopup::onDownloadFail() {
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

ThumbnailPopup* ThumbnailPopup::create(int id) {
    auto ret = new ThumbnailPopup();
    ret->m_levelID = id;
    if (ret && ret->initAnchored(395, 225, -1, "GJ_square05.png")) {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

class $modify(LevelInfoLayer2, LevelInfoLayer) {
    void onThumbnailButton(CCObject * target) {
        int id = m_level->m_levelID;
        ThumbnailPopup::create(id)->show();
    }

    bool init(GJGameLevel * p0, bool p1) {
        if (!LevelInfoLayer::init(p0, p1)) return false;

        auto sprite = CCSprite::create("thumbnailButton.png"_spr);
        auto button = CCMenuItemSpriteExtra::create(
            sprite, this, menu_selector(LevelInfoLayer2::onThumbnailButton)
        );
        button->setID("thumbnail-button");

        if (auto menu = getChildByID("left-side-menu")) {
            if (Mod::get()->getSettingValue<bool>("thumbnailButton")) {
                menu->addChild(button);
                menu->updateLayout();
            }
        }

        return true;
    }
};
