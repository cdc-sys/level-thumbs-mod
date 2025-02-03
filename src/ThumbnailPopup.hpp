#pragma once

#include <Geode/utils/web.hpp>

using namespace geode::prelude;

class ThumbnailPopup : public Popup<int> {
protected:
    std::unordered_set<Ref<CCTouch>> m_touches;
    float m_initialDistance;
    float m_initialScale;
    bool wasZooming=false;
    bool m_isScreenshotPreview;
    CCPoint m_touchMidPoint;
    int m_levelID;
    float m_maxHeight = 220;
    EventListener<web::WebTask> m_downloadListener;
    LoadingCircle* m_loadingCircle = LoadingCircle::create();
    CCMenuItemSpriteExtra* m_downloadBtn;
    CCMenuItemSpriteExtra* m_infoBtn;
    CCClippingNode* m_clippingNode;
    Ref<CCImage> m_image;
    CCLabelBMFont* m_theFunny;

    bool setup(int id) override;
    void onDownloadFinished(CCSprite* sprite);
    void onDownloadFail();
    void imageCreationFinished(CCImage* image);
    void onDownload(CCObject* sender);
    void openDiscordServerPopup(CCObject* sender);
    void onOpenFolder(CCObject* sender);
    void recenter(CCObject* sender);
    bool ccTouchBegan(cocos2d::CCTouch*pTouch,cocos2d::CCEvent* event) override;
    void ccTouchMoved(cocos2d::CCTouch*pTouch,cocos2d::CCEvent* event) override;
    void ccTouchEnded(cocos2d::CCTouch*pTouch,cocos2d::CCEvent* event) override;

public:
    static ThumbnailPopup* create(int id,bool screenshotPreview=false);
};