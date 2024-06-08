#pragma once

#include <Geode/utils/web.hpp>

using namespace geode::prelude;

class ThumbnailPopup : public Popup<int> {
protected:
	bool setup(int id) override;
	bool fetched = false;
	bool fetchFailed = false;
	EventListener<web::WebTask> downloadListener;
	int levelID;
	LoadingCircle* loadingCircle = LoadingCircle::create();
	CCTexture2D* texture = nullptr;
	CCMenuItemSpriteExtra* downloadBtn;

	void onDownloadFinished(CCSprite* sprite);
	void onDownloadFail();
	void imageCreationFinished(CCImage* image);
	void onDownload(CCObject*sender);
	void openDiscordServerPopup();

public:
	static ThumbnailPopup* create(int id);
};