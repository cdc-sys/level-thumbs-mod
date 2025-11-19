#pragma once
#include "Geode/cocos/CCDirector.h"
#include "Geode/cocos/cocoa/CCObject.h"
#include "Geode/cocos/label_nodes/CCLabelBMFont.h"
#include "Geode/cocos/layers_scenes_transitions_nodes/CCLayer.h"
#include "Geode/cocos/menu_nodes/CCMenuItem.h"
#include "Geode/utils/cocos.hpp"
#include <Geode/Geode.hpp>
#include <Geode/binding/CCMenuItemSpriteExtra.hpp>

using namespace geode::prelude;

using namespace cocos2d;
using namespace cocos2d::extension;

class ConfirmAlertLayer : public CCLayerColor {
	CCMenu* m_buttonMenu = nullptr;
	int m_controlConnected = -1;
	CCLayer* m_mainLayer = nullptr;
	int m_ZOrder = 0;
	bool m_noElasticity = false;
	bool m_reverseKeyBack = false;
	CCLayer* m_scrollingLayer = nullptr;
	ButtonSprite* m_button2 = nullptr;
	ButtonSprite* m_button1 = nullptr;
	CCMenuItemSpriteExtra* m_button1Extra = nullptr;
	CCMenuItemSpriteExtra* m_button2Extra = nullptr;
	int m_joystickConnected = -1;
	bool m_containsBorder = 0;
	bool m_noAction = false;
	float m_timer = 10;
	bool m_understood = false;
	CCLabelBMFont* m_buttonTimer;

	std::function<void(bool)> m_callback;
	inline void sendAndCleanup(bool alert) {
		m_callback(alert);
		//CCDirector::sharedDirector()->getTouchDispatcher()->setForcePrio(false);
		//CCDirector::sharedDirector()->getTouchDispatcher()->unregisterForcePrio(this);
		removeFromParentAndCleanup(true);
		setTouchEnabled(false);
		setKeypadEnabled(false);
		setKeyboardEnabled(false);
	}
	bool init(char const* title, gd::string caption, char const* button1, char const* button2, float width, bool border, float height) {
		if (!cocos2d::CCLayerColor::initWithColor(ccc4(0,0,0,150))) return false;
		m_containsBorder = border;

		float checkboxMargin = 20;
		
		// handle touch prio
		//cocos2d::CCTouchDispatcher::get()->registerForcePrio(this, 2);
		//this->registerWithTouchDispatcher();
		
		//this->setTouchPriority(-999);
		handleTouchPriority(this,true);

		setTouchEnabled(true);
		setKeypadEnabled(true);
		setKeyboardEnabled(true);
		m_reverseKeyBack = false;

		auto windowSize = CCDirector::sharedDirector()->getWinSize();

		m_mainLayer = cocos2d::CCLayer::create();
		addChild(m_mainLayer);

		TextArea* textArea;
		if (!m_containsBorder) {
			textArea = TextArea::create(caption, "chatFont.fnt", 1.0, width - 60.0, CCPointMake(0.5, 0.5), 20.0, false);
			m_mainLayer->addChild(textArea, 3);
			height = textArea->m_obRect.size.height + 120;
			height = std::max(height, 140.0f);
		}
		// honestly this is there but not added anywhere what
		// auto layerColor = CCLayerColor::create(ccc4(0,0,0,255), width, height);
		// auto layerColorPos = CCPointMake(windowSize.width - width, windowSize.height - height) * 0.5;
		// layerColor->setPosition(layerColorPos);
		// layerColor->setVisible(false);

		// auto layerColor2 = CCLayerColor::create(ccc4(150,150,150,255), width + 16, height + 16);
		// auto layerColor2Pos = layerColorPos - CCPointMake(8, 8);
		// layerColor2->setPosition(layerColor2Pos);
		// layerColor2->setVisible(false);

		auto scale9 = CCScale9Sprite::create("square01_001.png", CCRectMake(0, 0, 94, 94));
		scale9->setContentSize(CCSizeMake(width, height+checkboxMargin));
		auto scale9Pos = CCPointMake(windowSize.width*0.5, windowSize.height*0.5);
		scale9->setPosition(scale9Pos);
		m_mainLayer->addChild(scale9, 1);

		auto titleText = CCLabelBMFont::create(title, "goldFont.fnt");
		titleText->setAnchorPoint(CCPointMake(0.5, 1.0));
		titleText->setPosition(windowSize.width*0.5, (windowSize.height - height) * 0.5 + height - 15 + checkboxMargin/2);
		titleText->setScale(0.9);
		m_mainLayer->addChild(titleText, 3);

		if (!m_containsBorder) {
			textArea->setPosition(scale9Pos + CCPointMake(0, 5 + checkboxMargin/2));
		}

		m_buttonMenu = CCMenu::create();
		m_mainLayer->addChild(m_buttonMenu, 2);



		m_button1 = ButtonSprite::create(button1, 90, 0, 1.0, false);
		auto button1Menu = CCMenuItemSpriteExtra::create(m_button1, nullptr, this, SEL_MenuHandler(&ConfirmAlertLayer::onBtn1));
		m_button1Extra = button1Menu;
		m_buttonMenu->addChild(button1Menu);


		CCMenuItemSpriteExtra* button2Menu;
		if (!button2) m_noAction = true;
		else {
			m_button2 = ButtonSprite::create(button2, 90, 0, 1.0, false);
			button2Menu = CCMenuItemSpriteExtra::create(m_button2, nullptr, this, SEL_MenuHandler(&ConfirmAlertLayer::onBtn2));
			m_button2Extra = button2Menu;
			m_buttonMenu->addChild(button2Menu);
		}

		float menuLength = m_button1->getContentSize().width;
		menuLength += m_button2->getContentSize().width;
		auto menuCount = m_buttonMenu->getChildren()->count();
		if (menuCount > 1) {
			float padding = (width - menuLength) * 0.5;
			padding = std::min(padding, 15.0f);
			m_buttonMenu->alignItemsHorizontallyWithPadding(padding);
		}

		m_buttonMenu->setPosition(CCPointMake(windowSize.width * 0.5, (windowSize.height - height) * 0.5 + 30.0 - checkboxMargin/2));

		// timed label implementation thing
		auto readThisMenu = CCMenu::create();
		
		auto readThisLabel = CCLabelBMFont::create("I understand the consequences of this action.","bigFont.fnt");
		auto readThisToggle = CCMenuItemExt::createToggler(CCSprite::createWithSpriteFrameName("GJ_checkOn_001.png"),CCSprite::createWithSpriteFrameName("GJ_checkOff_001.png"),[this](CCObject* sender){
			m_understood = !m_understood;
		});
		
		readThisToggle->setScale(0.5);
		readThisLabel->limitLabelWidth(241, 0.5, 0.0001);

		// offset
		readThisLabel->setPositionX(15.f);
		readThisToggle->setPositionX(-readThisLabel->getScaledContentWidth()/2);

		readThisMenu->addChild(readThisLabel);
		readThisMenu->addChild(readThisToggle);

		readThisMenu->setPosition(windowSize.width/2,(windowSize.height - height) * 0.5 + 33.f + checkboxMargin);

		m_mainLayer->addChild(readThisMenu);
		readThisMenu->setZOrder(1); // this is stupid, and stupid, and stupid

		this->scheduleUpdate();

		return true;
	}

	void update(float dt) override {
		if (!m_button2Extra) return;
		bool buttonEnabled = false;
		bool showTimer = true;

		if (floor(m_timer) > 0){
			m_timer -= dt;
		}

		// unfortunately have to do this so it doesnt flicker
		if (floor(m_timer) <= 0){
			buttonEnabled = true;
			showTimer = false;
		}

		if (!m_understood) buttonEnabled = false;

		if (buttonEnabled){
			if (m_buttonTimer) m_buttonTimer->setVisible(false);
			m_button2Extra->setEnabled(true);
			m_button2->setOpacity(255);
			m_button2->setColor({255,255,255});
			m_button2->setCascadeOpacityEnabled(false);
			m_button2->setCascadeColorEnabled(false);
		} else {
			if (!m_buttonTimer){
				m_buttonTimer = CCLabelBMFont::create(fmt::format("{}",floor(m_timer)).c_str(),"bigFont.fnt");
				m_button2->addChild(m_buttonTimer);
				m_buttonTimer->setPosition(m_button2->m_label->getPosition());
				m_buttonTimer->setZOrder(999);
				m_buttonTimer->setScale(0.8);
			} else {
				m_buttonTimer->setVisible(showTimer);
				m_buttonTimer->setString(fmt::format("{}",floor(m_timer)).c_str());
			}
			m_button2Extra->setEnabled(false);
			m_button2->setCascadeOpacityEnabled(true);
			m_button2->setOpacity(150);
			m_button2->setCascadeColorEnabled(true);
			m_button2->setColor({150,150,150});
		}
	}
	bool ccTouchBegan(CCTouch* touch, CCEvent* event) override {
		return true;
	}
	void keyBackClicked() override {
		setKeypadEnabled(false);
		setKeyboardEnabled(false);
		sendAndCleanup(m_reverseKeyBack);
	}
	void registerWithTouchDispatcher() override {
		CCDirector::sharedDirector()->getTouchDispatcher()->addTargetedDelegate(this, -500, true);
	}
	void keyDown(enumKeyCodes keyCode) override {
		if (keyCode == 0x3ec) sendAndCleanup(true);
		else if (keyCode == KEY_Space && !m_noAction) return;
		else CCLayer::keyDown(keyCode);
	}
	void onBtn1(cocos2d::CCObject*) {
		sendAndCleanup(false);
	}
	void onBtn2(cocos2d::CCObject*) {
		sendAndCleanup(true);
	}

	public:
	CCNode* m_scene = nullptr;
	static ConfirmAlertLayer* create(std::function<void(bool)> callback,char const* title, gd::string caption, char const* button1, char const* button2) {
		return create(callback,title, caption, button1, button2, 350.0, false, 0.0);
	}
	static ConfirmAlertLayer* create(std::function<void(bool)> callback,char const* title, gd::string caption, char const* button1, char const* button2, float width) {
		return create(callback, title, caption, button1, button2, width, false, 0.0);
	}
	static ConfirmAlertLayer* create(std::function<void(bool)> callback,char const* title, gd::string caption, char const* button1, char const* button2, float width, bool border, float height) {
		auto ret = new ConfirmAlertLayer(); 
		ret->m_callback = callback;
		if (ret && ret->init(title, caption, button1, button2, width, border, height)) {
			ret->autorelease();
			return ret;
		}
		CC_SAFE_DELETE(ret);
		return NULL;
	}
	void show() {
		if (!m_noElasticity) {
			m_mainLayer->setScale(0.1);
			auto scaleTo = CCScaleTo::create(0.5, 1);
			auto elasticOut = CCEaseElasticOut::create(scaleTo, 0.6);
			m_mainLayer->runAction(elasticOut);
		}

		int zOrder;
		auto runningScene = m_scene;
		if (!runningScene) {
			runningScene = CCDirector::sharedDirector()->getRunningScene();
			m_ZOrder = CCDirector::sharedDirector()->getRunningScene()->getHighestChildZ() + 1;
		}
		m_ZOrder = std::max(105, m_ZOrder);
		runningScene->addChild(this, m_ZOrder);

		if (!m_noElasticity) {
			auto oldOpacity = getOpacity();
			setOpacity(0);
			runAction(CCFadeTo::create(0.14, oldOpacity));
		}

		setVisible(true);
	}
};
