#pragma once
#ifndef _LOADINGOVERLAY_H_
#define _LOADINGOVERLAY_H_
#include <Geode/Geode.hpp>

using namespace geode::prelude;

class LoadingOverlay : public CCLayer {
    bool init() override;
    bool ccTouchBegan(CCTouch *pTouch, CCEvent *pEvent) override;
    CCLabelBMFont *loadingTextLabel = nullptr;
    CCLayerColor *layerColor;
    LoadingCircle *loadingCircle;
    void keyBackClicked() override;
    void registerWithTouchDispatcher() override;
    int pressCount = 0;

public:
    void changeStatus(const char *status);
    void show();
    void fadeOut();
    static LoadingOverlay *create(const char *status = nullptr);
};
#endif