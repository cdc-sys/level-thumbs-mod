#include <Geode/Geode.hpp>

using namespace geode::prelude;

#include <Geode/modify/LevelInfoLayer.hpp>
#include <Geode/utils/web.hpp>
#include "ThumbnailPopup.hpp"
#include "utils.h"
void ThumbnailPopup::onDownload(CCObject*sender){
	std::string URL = fmt::format("https://cdc-sys.github.io/level-thumbnails/thumbs/{}.png", this->levelID);
	CCApplication::sharedApplication()->openURL(URL.c_str());
}
//There is probably some bad/inneficient code in here.
bool ThumbnailPopup::setup(int id) {
	m_noElasticity = false;
	auto winSize = CCDirector::sharedDirector()->getWinSize();

	this->setTitle("Thumbnail");

	//bool fetched = false;
	//bool fetchFailed = false;

	//LoadingCircle* loadingCircle = LoadingCircle::create();
	CCMenu* downloadMenu = CCMenu::create();
	CCSprite* downloadSprite = CCSprite::createWithSpriteFrameName("GJ_downloadBtn_001.png");
	downloadBtn = CCMenuItemSpriteExtra::create(downloadSprite,this,menu_selector(ThumbnailPopup::onDownload));
	downloadBtn->setEnabled(false);
	downloadBtn->setColor({125,125,125});
	downloadMenu->setPosition({this->m_mainLayer->getContentWidth()-28,30});
	downloadMenu->addChild(downloadBtn);
	this->m_mainLayer->addChild(downloadMenu);
	ThumbnailPopup::loadingCircle->setParentLayer(this->m_mainLayer);
	ThumbnailPopup::loadingCircle->setPosition({ -70,-40 });
	ThumbnailPopup::loadingCircle->setScale(1.f);
	ThumbnailPopup::loadingCircle->show();
	
	//auto texture = nullptr;
	auto txtr = CCTextureCache::get()->textureForKey(fmt::format("thumb-{}", levelID).c_str());
	
	if (txtr && !Mod::get()->getSettingValue<bool>("disableCache")) {
		ThumbnailPopup::loadingCircle->fadeAndRemove();
		ThumbnailPopup::onDownloadFinished(CCSprite::createWithTexture(txtr));
	}
	else {
		this->retain();
		std::string URL = fmt::format("https://cdc-sys.github.io/level-thumbnails/thumbs/{}.png", levelID);
		geode::log::info("{}",URL);
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
				std::string theKey = fmt::format("thumb-{}", levelID);
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
					this->onDownloadFail();
					ThumbnailPopup::fetchFailed = true;
				});
	}

	return true;
}

void ThumbnailPopup::onDownloadFinished(CCSprite* sprite) {
	// thanks for fucking this up sheepdotcom
	downloadBtn->setEnabled(true);
	downloadBtn->setColor({255,255,255});
	CCSprite* image = sprite;
	image->setScale(0.65f / levelthumbs::getQualityMultiplier());
	image->setPosition({(this->m_mainLayer->getContentWidth()/2),(this->m_mainLayer->getContentHeight()/2)-10.f});
	this->m_mainLayer->addChild(image);
}
void ThumbnailPopup::onDownloadFail() {
	// thanks for the image cvolton ;)
	CCSprite* image = CCSprite::create("noThumb.png"_spr);
	image->setScale(0.65f );
	image->setPosition({(this->m_mainLayer->getContentWidth()/2),(this->m_mainLayer->getContentHeight()/2)-10.f});
	this->m_mainLayer->addChild(image);
}

ThumbnailPopup* ThumbnailPopup::create(int id) {
	auto ret = new ThumbnailPopup();
	ret->levelID = id;
	if (ret && ret->initAnchored(420, 240, -1, "GJ_square01.png")) {
		ret->autorelease();
		return ret;
	}
	CC_SAFE_DELETE(ret);
	return nullptr;
}

//Copying so much code from main.cpp be like


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