#pragma once

#include <Geode/utils/web.hpp>

using namespace geode::prelude;

class ThumbnailPopup : public Popup<int> {
protected:
	bool fetched = false;
	bool fetchFailed = false;
	int levelID;
	EventListener<web::WebTask> downloadListener;
	LoadingCircle* loadingCircle = LoadingCircle::create();
	CCTexture2D* texture = nullptr;
	CCMenuItemSpriteExtra* downloadBtn;

	bool setup(int id) override;
	void onDownloadFinished(CCSprite* sprite);
	void onDownloadFail();
	void imageCreationFinished(CCImage* image);
	void onDownload(CCObject*sender);
	void openDiscordServerPopup();

public:
	static ThumbnailPopup* create(int id);
};