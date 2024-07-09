#include <Geode/Geode.hpp>

using namespace geode::prelude;

#include <Geode/modify/LevelInfoLayer.hpp>
#include <Geode/utils/web.hpp>
#include "ThumbnailPopup.hpp"
#include "utils.hpp"
#include "ImageCache.hpp"

void ThumbnailPopup::onDownload(CCObject*sender){
	std::string URL = fmt::format("https://raw.githubusercontent.com/cdc-sys/level-thumbnails/main/thumbs/{}.png", m_levelID);
	CCApplication::sharedApplication()->openURL(URL.c_str());
}

void ThumbnailPopup::openDiscordServerPopup(){
	if (Mod::get()->getSavedValue<bool>("discord-ad-shown")){
		return;
	}
	else{
		Mod::get()->setSavedValue<bool>("discord-ad-shown",true);
	}
	createQuickPopup(
        "Uh Oh!",
        "Hm.. This level seems to not have a <cj>Thumbnail</c>...\n"
        "Worry not! You can join our <cg>Discord Server</c> and submit a thumbnail <cy>YOURSELF!</c>\n<cb>(Don't worry, you can always find the server link in the mod about page..)</c>",
        "No Thanks", "JOIN!",
        [this](auto, bool btn2) {
            if (btn2) {
                CCApplication::sharedApplication()->openURL("https://discord.gg/K6M4RduZxY");
            }
        }
    );
}

bool ThumbnailPopup::setup(int id) {
	m_noElasticity = false;
	auto winSize = CCDirector::sharedDirector()->getWinSize();

	setTitle("Thumbnail");

	CCMenu* downloadMenu = CCMenu::create();
	CCSprite* downloadSprite = CCSprite::createWithSpriteFrameName("GJ_downloadBtn_001.png");
	m_downloadBtn = CCMenuItemSpriteExtra::create(downloadSprite,this,menu_selector(ThumbnailPopup::onDownload));
	m_downloadBtn->setEnabled(false);
	m_downloadBtn->setColor({125,125,125});
	downloadMenu->setPosition({m_mainLayer->getContentWidth()-28,30});
	downloadMenu->addChild(m_downloadBtn);
	m_mainLayer->addChild(downloadMenu);
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
				m_loadingCircle->fadeAndRemove();
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

void ThumbnailPopup::onDownloadFinished(CCSprite* sprite) {
	// thanks for fucking this up sheepdotcom
	m_downloadBtn->setEnabled(true);
	m_downloadBtn->setColor({255,255,255});
	CCSprite* image = sprite;
	image->setScale(0.65f / levelthumbs::getQualityMultiplier());
	image->setPosition({(m_mainLayer->getContentWidth()/2),(m_mainLayer->getContentHeight()/2)-10.f});
	m_mainLayer->addChild(image);
}

void ThumbnailPopup::onDownloadFail() {
	// thanks for the image cvolton ;)
	CCSprite* image = CCSprite::create("noThumb.png"_spr);
	image->setScale(0.65f );
	image->setPosition({(m_mainLayer->getContentWidth()/2),(m_mainLayer->getContentHeight()/2)-10.f});
	m_mainLayer->addChild(image);
}

ThumbnailPopup* ThumbnailPopup::create(int id) {
	auto ret = new ThumbnailPopup();
	ret->m_levelID = id;
	if (ret && ret->initAnchored(420, 240, -1, "GJ_square01.png")) {
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
