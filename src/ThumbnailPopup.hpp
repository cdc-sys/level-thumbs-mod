#pragma once

#include <Geode/utils/web.hpp>

using namespace geode::prelude;

class ThumbnailPopup : public Popup<int> {
protected:
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
    void recenter(CCObject* sender);

public:
    static ThumbnailPopup* create(int id);
};