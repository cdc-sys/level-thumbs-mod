#include <Geode/Geode.hpp>
#include <variant>

using namespace geode::prelude;

#include <Geode/modify/LevelCell.hpp>
#include <Geode/utils/web.hpp>
#include "utils.hpp"

class $modify(MyLevelCell, LevelCell) {
	struct Fields{
		LoadingCircle* loadingIndicator;
		CCSprite* placeholderImage;
		CCSprite* separatorSprite;
		CCLabelBMFont* notAvailable;
		CCLayerColor* background;
		EventListener<web::WebTask> downloadListener;

		bool fetched = false;
		bool fetchFailed = false;
	};

	void loadCustomLevelCell() {
		LevelCell::loadCustomLevelCell();
		this->m_fields->background = getChildOfType<CCLayerColor>(this, 0);
		this->m_fields->background->setZOrder(-2);
		this->m_fields->separatorSprite = CCSprite::create(Mod::get()->getSettingValue<bool>("alternativeSpacer") ? "streakb_01_001.png" : "square02_001.png");
		
		auto mainLayer = this->getChildByID("main-layer");
		auto mainMenu = mainLayer->getChildByID("main-menu");
		auto viewButton = mainMenu->getChildByID("view-button");

		this->m_fields->loadingIndicator = LoadingCircle::create();
		this->m_fields->loadingIndicator->setParentLayer(this);

		this->m_fields->separatorSprite->setPosition({(this->m_compactView ? 241.f : 215.f),(this->m_compactView ? 25.f : 45.f)});
		if (Mod::get()->getSettingValue<bool>("alternativeSpacer")) {
			this->m_fields->separatorSprite->setScaleX((this->m_compactView ? 1.875f : 1.95f));
			this->m_fields->separatorSprite->setScaleY((this->m_compactView ? 6.275f : 11.375f));
			this->m_fields->separatorSprite->setSkewX({ (this->m_compactView ? 49.f : 66.f) });

			this->m_fields->separatorSprite->setColor(ccColor3B{ 0, 0, 0 });
		} else {
			this->m_fields->separatorSprite->setScaleX(0.175f);
			this->m_fields->separatorSprite->setScaleY((this->m_compactView ? 0.625f : 1.125f));
			this->m_fields->separatorSprite->setSkewX({ (this->m_compactView ? 49.f : 66.f) });
		}

		this->m_fields->separatorSprite->setZOrder(-2);
		this->m_fields->separatorSprite->setOpacity(50);
		this->m_fields->separatorSprite->setVisible(false);	


		this->addChild(this->m_fields->separatorSprite);
		this->m_fields->loadingIndicator->setPosition({viewButton->getPositionX() + (this->m_compactView ? -42.f : 20.f),viewButton->getPositionY() + (this->m_compactView ? 0.f : -30.f)});
		this->m_fields->loadingIndicator->setScale(0.3f);
		this->m_fields->loadingIndicator->show();

		this->startDownload();
	}

	void startDownload() {
		auto txtr = CCTextureCache::get()->textureForKey(fmt::format("thumb-{}",(int)this->m_level->m_levelID).c_str());
		if (txtr && !Mod::get()->getSettingValue<bool>("disableCache")) {
			this->onDownloadFinished(CCSprite::createWithTexture(txtr));
			return;
		}
		std::string URL = fmt::format("https://raw.githubusercontent.com/cdc-sys/level-thumbnails/main/thumbs/{}.png",(int)this->m_level->m_levelID);

		auto downloadProgressText = CCLabelBMFont::create("0%","bigFont.fnt");
		downloadProgressText->setPosition({50,50});
		this->addChild(downloadProgressText);

		auto req = web::WebRequest();
		m_fields->downloadListener.bind([this,downloadProgressText](web::WebTask::Event* e){
			if (auto res = e->getValue()){
				if (!res->ok()) {
					this->onDownloadFailed();
					this->m_fields->fetchFailed = true;
				} else {
					downloadProgressText->removeFromParent();
					auto data = res->data();
					std::thread imageThread = std::thread([data,this](){
						auto image = Ref(new CCImage());
						image->initWithImageData(const_cast<uint8_t*>(data.data()),data.size());
						geode::Loader::get()->queueInMainThread([image,this](){
							this->imageCreationFinished(image);
						});
					});
				imageThread.detach();
				}
			} else if (e->isCancelled()){
				geode::log::warn("Exited before letting it finish fuck you");
			} else if (e->getProgress()){
				if (!e->getProgress()->downloadProgress().has_value()){
					return;
				}
				geode::log::info("{}",e->getProgress()->downloadProgress());
				downloadProgressText->setString(fmt::format("{}%",round(e->getProgress()->downloadProgress().value())).c_str());
			}
		});
		auto downloadTask = req.get(URL);
		m_fields->downloadListener.setFilter(downloadTask);
	}
	void destructor(){
		geode::log::info("destructor");
	}
	void imageCreationFinished(CCImage* image){
		std::string theKey = fmt::format("thumb-{}",(int)this->m_level->m_levelID);
		auto texture = CCTextureCache::get()->addUIImage(image,theKey.c_str());
		this->onDownloadFinished(CCSprite::createWithTexture(texture));
		this->m_fields->fetched = true;
		image->release();
		this->autorelease();
	}
	void onDownloadFailed() {
		this->m_fields->loadingIndicator->fadeAndRemove();
		this->m_fields->separatorSprite->removeFromParent();
	}

	void onDownloadFinished(CCSprite* sprite) {
		this->m_fields->loadingIndicator->fadeAndRemove();
		CCSprite* image = sprite;
		CCPoint rectangle[4] = {
    		CCPoint(0, 0),
    		CCPoint(31.8f, 90),
    		CCPoint((this->m_compactView ? 220.f : 160.f), 90),
    		CCPoint((this->m_compactView ? 220.f : 160.f), 0)
		};

		auto clippingNode = CCClippingNode::create();

		auto mask = CCDrawNode::create();
		mask->drawPolygon(rectangle, 4, ccc4FFromccc3B({255, 0, 0}), 1, ccc4FFromccc3B({255, 125, 0}));
		clippingNode->setStencil(mask);
		clippingNode->addChild(image);
		image->setScale((this->m_compactView ? 0.5f : 0.332f)/levelthumbs::getQualityMultiplier());
		image->setPosition({image->getScaledContentSize().width/2,image->getScaledContentSize().height/2 + (this->m_compactView ? -32.5f : 0.f)});
		this->addChild(clippingNode);
		clippingNode->setPosition({(this->m_compactView ? 235.f : 205.f),0});
		clippingNode->setScale((this->m_compactView ? 0.555f : 1.f));
		clippingNode->setZOrder(-1);
		this->m_fields->separatorSprite->setVisible(true);
	}
};
