#pragma once

using namespace geode::prelude;

class ThumbnailPopup : public Popup<int> {
protected:
	bool setup(int id) override;
	bool fetched = false;
	bool fetchFailed = false;
	int levelID;
	LoadingCircle* loadingCircle = LoadingCircle::create();
	CCTexture2D* texture = nullptr;

	void onDownloadFinished(CCSprite* sprite);
	void onDownloadFail();

public:
	static ThumbnailPopup* create(int id);
};