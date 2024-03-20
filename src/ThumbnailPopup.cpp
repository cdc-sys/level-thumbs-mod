#include <Geode/Geode.hpp>

using namespace geode::prelude;

#include <Geode/modify/LevelInfoLayer.hpp>
#include <Geode/utils/web.hpp>
#include "ThumbnailPopup.hpp"

//There is probably some bad/inneficient code in here.
bool ThumbnailPopup::setup(int id) {
	m_noElasticity = true;
	auto winSize = CCDirector::sharedDirector()->getWinSize();

	this->setTitle("Testing");

	//bool fetched = false;
	//bool fetchFailed = false;

	//LoadingCircle* loadingCircle = LoadingCircle::create();
	ThumbnailPopup::loadingCircle->setParentLayer(this);
	ThumbnailPopup::loadingCircle->setPosition({ 0,0 });
	ThumbnailPopup::loadingCircle->setScale(0.5f);
	ThumbnailPopup::loadingCircle->show();
	
	//auto texture = nullptr;
	auto txtr = CCTextureCache::get()->textureForKey(fmt::format("thumb-{}", id).c_str());
	
	if (txtr && !Mod::get()->getSettingValue<bool>("disableCache")) {
		ThumbnailPopup::loadingCircle->fadeAndRemove();
		ThumbnailPopup::onDownloadFinished(CCSprite::createWithTexture(txtr));
	}
	else {
		this->retain();
		std::string URL = fmt::format("https://cdc-sys.github.io/level-thumbnails/thumbs/{}.png", id);
		web::AsyncWebRequest()
			.fetch(URL)
			.bytes()
			.then([=, this](geode::ByteVector const& data) {
			if (ThumbnailPopup::fetchFailed == true) {
				ThumbnailPopup::fetched = true;
			}
			else {
				auto image = Ref(new CCImage());
				image->initWithImageData(const_cast<uint8_t*>(data.data()), data.size());
				std::string theKey = fmt::format("thumb-{}", id);
				CCTextureCache::get()->removeTextureForKey(theKey.c_str());
				ThumbnailPopup::texture = CCTextureCache::get()->addUIImage(image, theKey.c_str());
				ThumbnailPopup::loadingCircle->fadeAndRemove();
				ThumbnailPopup::onDownloadFinished(CCSprite::createWithTexture(ThumbnailPopup::texture));
				ThumbnailPopup::fetched = true;
				image->release();
				this->autorelease();
			}
				})
			.expect([=, this](std::string const& error) {
					ThumbnailPopup::loadingCircle->fadeAndRemove();
					ThumbnailPopup::fetchFailed = true;
				});
	}

	return true;
}

void ThumbnailPopup::onDownloadFinished(CCSprite* sprite) {
	CCSprite* image = sprite;
	CCPoint rectangle[4] = {
		CCPoint(0, 0),
		CCPoint(31.8f, 90),
		CCPoint(160.f, 90),
		CCPoint(160.f, 0)
		//CCPoint(160.f, 400),
		//CCPoint(160.f, 0)
	};

	auto clippingNode = CCClippingNode::create();

	auto mask = CCDrawNode::create();
	mask->drawPolygon(rectangle, 4, ccc4FFromccc3B({ 255, 0, 0 }), 1, ccc4FFromccc3B({ 255, 125, 0 }));
	clippingNode->setStencil(mask);
	clippingNode->addChild(image);
	image->setScale(0.332f / 1);
	image->setPosition({ image->getScaledContentSize().width / 2,image->getScaledContentSize().height / 2 + 0.f });
	this->addChild(clippingNode);
	this->addChild(image);
	// 205, 235
	clippingNode->setPosition({ 205.f,0 });
	clippingNode->setScale(1.f);
	clippingNode->setZOrder(-1);
}

ThumbnailPopup* ThumbnailPopup::create(int id) {
	auto ret = new ThumbnailPopup();
	if (ret && ret->initAnchored(420, 240, -1, "GJ_square01.png")) {
		ret->autorelease();
		return ret;
	}
	CC_SAFE_DELETE(ret);
	return nullptr;
}

//Copying so much code from main.cpp be like
int getQualityMultiplier() {
	auto scaleFactor = CCDirector::sharedDirector()->getContentScaleFactor();
	switch ((int)scaleFactor) {
	case 1:
		return 4;
		break;
	case 2:
		return 2;
		break;
	case 4:
		return 1;
		break;
	}
	return 1;
}

class $modify(LevelInfoLayer2, LevelInfoLayer) {
	void onThumbnailButton(CCObject * target) {
		int id = this->m_level->m_levelID;
		ThumbnailPopup::create(id)->show();
	}

	bool init(GJGameLevel * p0, bool p1) {
		if (!LevelInfoLayer::init(p0, p1)) return false;

		auto sprite = CCSprite::create("thumbnailButton.png"_spr);
		auto button = CCMenuItemSpriteExtra::create(
			sprite, this, menu_selector(LevelInfoLayer2::onThumbnailButton)
		);
		button->setID("thumbnail-button");

		if (auto menu = this->getChildByID("left-side-menu")) {
			if (Mod::get()->getSettingValue<bool>("thumbnailButton")) {
				menu->addChild(button);
				menu->updateLayout();
			}
		}

		return true;
	}
};