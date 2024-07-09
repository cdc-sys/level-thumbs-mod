#pragma once

#include <Geode/utils/web.hpp>

using namespace geode::prelude;

class ThumbnailPopup : public Popup<int> {
protected:
	int m_levelID;
	EventListener<web::WebTask> m_downloadListener;
	LoadingCircle* m_loadingCircle = LoadingCircle::create();
	CCMenuItemSpriteExtra* m_downloadBtn;
	Ref<CCImage> m_image;

	bool setup(int id) override;
	void onDownloadFinished(CCSprite* sprite);
	void onDownloadFail();
	void imageCreationFinished(CCImage* image);
	void onDownload(CCObject*sender);
	void openDiscordServerPopup();

public:
	static ThumbnailPopup* create(int id);
};