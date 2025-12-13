#include "LoadingOverlay.hpp"
#include "Geode/cocos/CCDirector.h"
void LoadingOverlay::show() {
    auto scene = CCDirector::sharedDirector()->getRunningScene();
    layerColor->runAction(
        CCFadeTo::create(0.25f, 175));
    if (loadingTextLabel) {
        loadingTextLabel->runAction(
            CCFadeIn::create(0.25f));
    }
    scene->addChild(this);
}
void LoadingOverlay::changeStatus(const char *status) {
    if (status) {
        if (loadingTextLabel) {
            loadingTextLabel->setString(status);
        } else {
            auto winSize = CCDirector::sharedDirector()->getWinSize();
            loadingTextLabel = CCLabelBMFont::create(status, "goldFont.fnt");
            loadingTextLabel->setScale(0.6f);
            loadingTextLabel->setPosition({winSize.width / 2.f, winSize.height / 2.5f + 12.25f});
            this->addChild(loadingTextLabel);
            loadingCircle->setScale(0.5f);
            loadingCircle->setPosition({0, 12.25f});
        }
    } else {
        if (loadingTextLabel) {
            loadingTextLabel->removeFromParent();
            loadingTextLabel->release();
            loadingTextLabel = nullptr;
            loadingCircle->setScale(1.f);
            loadingCircle->setPosition({0, 0});
        }
    }
}
void LoadingOverlay::keyBackClicked(){
    return;
}
void LoadingOverlay::fadeOut() {
    layerColor->runAction(
        CCSequence::create(
            CCFadeTo::create(0.25f, 0),
            CallFuncExt::create([this](){
                this->removeFromParent();
            }),
            nullptr));
    if (loadingTextLabel) {
        loadingTextLabel->runAction(
            CCFadeTo::create(0.25f, 0));
    }
    loadingCircle->fadeAndRemove();
}
void LoadingOverlay::registerWithTouchDispatcher() {
    CCDirector::sharedDirector()->getTouchDispatcher()->addTargetedDelegate(this, -9999, true);
}
bool LoadingOverlay::init() {
    if (!CCLayer::init()) {
        return false;
    }
    auto winSize = CCDirector::sharedDirector()->getWinSize();
    this->registerWithTouchDispatcher();
    this->setTouchEnabled(true);
    this->setKeypadEnabled(true);
    //this->setTouchPriority(-9999);
    this->setZOrder(CCDirector::get()->getRunningScene()->getHighestChildZ()+1);
    //CCDirector::sharedDirector()
    // CCDirector::sharedDirector()->getTouchDispatcher()->registerForcePrio(this,-9999);
    cocos2d::CCTouchDispatcher::get()->registerForcePrio(this, 2);
	handleTouchPriority(this,true);

    // this->handler
    loadingCircle = LoadingCircle::create();
    loadingCircle->setScale((loadingTextLabel ? 0.5f : 1.f));
    if (loadingTextLabel) {
        loadingCircle->setPosition({0, 12.25f});
    }
    loadingCircle->setParentLayer(this);
    loadingCircle->show();
    layerColor = CCLayerColor::create({0, 0, 0});
    // layerColor->setOpacity(127.5);
    this->addChild(layerColor);
    if (loadingTextLabel) {
        loadingTextLabel->setScale(0.6f);
        loadingTextLabel->setPosition({winSize.width / 2.f, winSize.height / 2.5f + 12.25f});
        loadingTextLabel->setOpacity(0);
        this->addChild(loadingTextLabel);
    }
    return true;
}
LoadingOverlay *LoadingOverlay::create(const char *status) {
    auto ret = new LoadingOverlay();
    if (status) {
        ret->loadingTextLabel = CCLabelBMFont::create(status, "goldFont.fnt");
    }
    if (ret && ret->init()) {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}
bool LoadingOverlay::ccTouchBegan(CCTouch *pTouch, CCEvent *pEvent) {
    return true;
}