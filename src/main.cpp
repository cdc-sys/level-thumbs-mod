#include <Geode/Geode.hpp>
#include <variant>

using namespace geode::prelude;

#include <Geode/modify/LevelCell.hpp>
#include <Geode/utils/web.hpp>
#include "utils.hpp"
#include "ImageCache.hpp"

class $modify(MyLevelCell, LevelCell) {
    
    struct Fields{
        Ref<LoadingCircle> m_loadingIndicator;
        Ref<CCLayerColor> m_separator;
        Ref<CCLabelBMFont> m_downloadProgressText;
        EventListener<web::WebTask> m_downloadListener;
        Ref<CCLayerColor> m_background;
        SEL_SCHEDULE m_parentCheck;
        Ref<CCClippingNode> m_clippingNode;
        Ref<CCImage> m_image;
    };

    void loadCustomLevelCell() {
        LevelCell::loadCustomLevelCell();
        if(CCLayerColor* bg = getChildOfType<CCLayerColor>(this, 0)){
            m_fields->m_background = bg;
            bg->setZOrder(-2);
        }
        
        m_fields->m_clippingNode = CCClippingNode::create();

        auto mainLayer = getChildByID("main-layer");
        auto mainMenu = mainLayer->getChildByID("main-menu");
        auto viewButton = mainMenu->getChildByID("view-button");

        m_fields->m_loadingIndicator = LoadingCircle::create();
        m_fields->m_loadingIndicator->setParentLayer(this);

        m_fields->m_separator = CCLayerColor::create({0, 0, 0});
        m_fields->m_separator->setZOrder(-2);
        m_fields->m_separator->setOpacity(50);
        m_fields->m_separator->setScaleX(0.45f);
        m_fields->m_separator->setVisible(false);	
        m_fields->m_separator->ignoreAnchorPointForPosition(false);

        addChild(m_fields->m_separator);

        m_fields->m_loadingIndicator->setPosition({viewButton->getPositionX() + (m_compactView ? -42.f : 20.f), viewButton->getPositionY() + (m_compactView ? 0.f : -30.f)});
        m_fields->m_loadingIndicator->setScale(0.3f);
        m_fields->m_loadingIndicator->show();

        m_fields->m_downloadProgressText = CCLabelBMFont::create("0%","bigFont.fnt");
        m_fields->m_downloadProgressText->setPosition({352, 1});
        m_fields->m_downloadProgressText->setAnchorPoint({1, 0});
        m_fields->m_downloadProgressText->setScale(0.25f);
        m_fields->m_downloadProgressText->setOpacity(128);

        addChild(m_fields->m_downloadProgressText);

        m_fields->m_parentCheck = schedule_selector(MyLevelCell::checkParent);

        schedule(m_fields->m_parentCheck);

        retain();
        
        startDownload();
    }

    //hacky but we have no other choice
    void checkParent(float dt) {
        if(CCNode* node = getParent()){
            if(typeinfo_cast<DailyLevelNode*>(node)){
                setDailyAttributes();
            }
            unschedule(m_fields->m_parentCheck);
        }
    }

    void startDownload() {

        if(CCImage* image = ImageCache::get()->getImage(fmt::format("thumb-{}", (int)m_level->m_levelID))){
            m_fields->m_image = image;
            imageCreationFinished(m_fields->m_image);
            return;
        }

        std::string URL = fmt::format("{}/{}.png", Mod::get()->getSettingValue<std::string>("indexURL"), (int)m_level->m_levelID);

        auto req = web::WebRequest();
        m_fields->m_downloadListener.bind([this](web::WebTask::Event* e){
            if (auto res = e->getValue()){
                if (!res->ok()) {
                    onDownloadFailed();
                } else {
                    m_fields->m_downloadProgressText->removeFromParent();
                    auto data = res->data();
                    std::thread imageThread = std::thread([data, this](){
                        m_fields->m_image = new CCImage();
                        m_fields->m_image->autorelease();
                        m_fields->m_image->initWithImageData(const_cast<uint8_t*>(data.data()),data.size());
                        geode::Loader::get()->queueInMainThread([this](){
                            ImageCache::get()->addImage(m_fields->m_image, fmt::format("thumb-{}", (int)m_level->m_levelID));
                            imageCreationFinished(m_fields->m_image);
                        });
                    });
                    imageThread.detach();
                }
            } else if (web::WebProgress* progress = e->getProgress()){
                if (!progress->downloadProgress().has_value()){
                    return;
                }
                m_fields->m_downloadProgressText->setString(fmt::format("{}%",round(e->getProgress()->downloadProgress().value())).c_str());
            } else if (e->isCancelled()){
                geode::log::warn("Exited before finishing");
            } 
        });
        auto downloadTask = req.get(URL);
        m_fields->m_downloadListener.setFilter(downloadTask);
    }

    void imageCreationFinished(CCImage* image){

        CCTexture2D* texture = new CCTexture2D();
        texture->autorelease();
        texture->initWithImage(image);
        onDownloadFinished(CCSprite::createWithTexture(texture));
    }

    void onDownloadFailed() {
        m_fields->m_separator->removeFromParent();
        handleFinish();
    }

    void handleFinish(){
        m_fields->m_loadingIndicator->fadeAndRemove();
        m_fields->m_downloadProgressText->removeFromParent();
        release();
    }

    void onDownloadFinished(CCSprite* image) {

        float imgScale = m_fields->m_background->getContentSize().height / image->getContentSize().height;

        image->setScale(imgScale);

        float separatorXMul = 1;

        if(m_compactView){
            image->setScale(image->getScale()*1.3f);
            separatorXMul = 0.75;
        }

        CCLayerColor* rect = CCLayerColor::create({255, 255, 255});
        
        float angle = 18;

        CCSize scaledImageSize = {image->getScaledContentSize().width, image->getContentSize().height * imgScale};

        rect->setSkewX(angle);
        rect->setContentSize(scaledImageSize);
        rect->setAnchorPoint({1, 0});
        
        m_fields->m_separator->setSkewX(angle*2);
        m_fields->m_separator->setContentSize(scaledImageSize);
        m_fields->m_separator->setAnchorPoint({1, 0});

        m_fields->m_clippingNode->setStencil(rect);
        m_fields->m_clippingNode->addChild(image);
        m_fields->m_clippingNode->setContentSize(scaledImageSize);
        m_fields->m_clippingNode->setAnchorPoint({1, 0});
        m_fields->m_clippingNode->setPosition({m_fields->m_background->getContentSize().width, 0.3f});

        float scale =  m_fields->m_background->getContentSize().height / m_fields->m_clippingNode ->getContentSize().height;

        rect->setScale(scale);
        image->setPosition({m_fields->m_clippingNode ->getContentSize().width/2, m_fields->m_clippingNode ->getContentSize().height/2});

        m_fields->m_separator->setPosition({m_fields->m_background->getContentSize().width - m_fields->m_separator->getContentSize().width/2 - (20 * separatorXMul), 0.3f});
        m_fields->m_separator->setVisible(true);

        m_fields->m_clippingNode->setZOrder(-1);

        addChild(m_fields->m_clippingNode);

        if(typeinfo_cast<DailyLevelNode*>(getParent())){
            setDailyAttributes();
        }

        handleFinish();
    }

    void setDailyAttributes(){
        
        float dailyMult = 1.22;

        m_fields->m_separator->setScaleX(0.45 * dailyMult);
        m_fields->m_separator->setScaleY(dailyMult);
        m_fields->m_separator->setPosition({m_fields->m_background->getContentSize().width - (m_fields->m_separator->getContentSize().width * dailyMult)/2 - 20 + 7, -7.9f});
        m_fields->m_clippingNode->setScale(dailyMult);
        m_fields->m_clippingNode->setPosition(m_fields->m_clippingNode->getPosition().x + 7, -7.9f);

        DailyLevelNode* dln = typeinfo_cast<DailyLevelNode*>(getParent());

        if(CCScale9Sprite* bg = typeinfo_cast<CCScale9Sprite*>(dln->getChildByID("background"))){

            CCScale9Sprite* border = CCScale9Sprite::create("GJ_square07.png");
            border->setContentSize(bg->getContentSize());
            border->setPosition(bg->getPosition());
            border->setColor(bg->getColor());
            border->setZOrder(5);
            border->setID("border"_spr);
            dln->addChild(border);
        }

        if(CCNode* node = dln->getChildByID("crown-sprite")){
            node->setZOrder(6);
        }
    }
};
