#include "Zoom.hpp"

#ifdef GEODE_IS_WINDOWS

#include <Geode/modify/CCMouseDispatcher.hpp>
#include <Geode/modify/CCEGLView.hpp>
#include <Geode/modify/CCScheduler.hpp>

Zoom* Zoom::instance = nullptr;

bool checkIfInside(CCLayer* layer, CCPoint pos){
    float x = layer->getPosition().x - layer->getContentSize().width * layer->getAnchorPoint().x;
    float y = layer->getPosition().y - layer->getContentSize().height * layer->getAnchorPoint().y;
    float width = layer->getContentSize().width;
    float height = layer->getContentSize().height;

    CCRect r = {x, y, width, height};

    CCPoint local = CCScene::get()->convertToNodeSpace(pos);

    return r.containsPoint(local);
}

void Zoom::doZoom(float y){

    float zoom = 0.1f * (y > 0 ? -1 : 1);
    auto pos = getMousePos();

    CCNode* thumbnailPopup = CCScene::get()->getChildByID("ThumbnailPopup");
    if (!thumbnailPopup) return;

    CCLayer* popupLayer = thumbnailPopup->getChildByType<CCLayer>(0);
    if(!popupLayer) return;

    CCNode* thumbnail = thumbnailPopup->getChildByIDRecursive("thumbnail");
    if(!thumbnail) return;

    if(!checkIfInside(popupLayer, pos)) return;

    CCSize contentSize = thumbnail->getContentSize();

    float oldScale = thumbnail->getScale();

    if(CCFloat* val = static_cast<CCFloat*>(thumbnail->getUserObject("new-scale"))){
        oldScale = val->getValue();
    }

    float newScale;
    float origScale = static_cast<CCFloat*>(thumbnail->getUserObject("scale"))->getValue();

    if (zoom < 0) {
        newScale = oldScale / (1 - zoom);
    } else {
        newScale = oldScale * (1 + zoom);
    }

    if (newScale < origScale * 0.75) {
        newScale = origScale * 0.75;
    }

    if (newScale > origScale * 100) {
        newScale = origScale * 100;
    }
    
    float currentScale = thumbnail->getScale();

    auto prevPos = thumbnail->convertToNodeSpace(pos);
    thumbnail->setScale(newScale);
    auto newPos = thumbnail->convertToWorldSpace(prevPos);
    thumbnail->setScale(currentScale);
    thumbnail->setUserObject("new-scale", CCFloat::create(newScale));

    thumbnail->stopAllActions();
    thumbnail->runAction(CCMoveTo::create(0.1, thumbnail->getPosition() + pos - newPos));
    thumbnail->runAction(CCScaleTo::create(0.1, newScale));
}

void Zoom::update(float dt){
    auto pos = getMousePos();
    auto lastMousePos = m_lastMousePos;

    CCNode* thumbnailPopup = CCScene::get()->getChildByID("ThumbnailPopup");
    if (!thumbnailPopup) return;

    CCLayer* popupLayer = thumbnailPopup->getChildByType<CCLayer>(0);
    if(!popupLayer) return;

    CCNode* thumbnail = thumbnailPopup->getChildByIDRecursive("thumbnail");
    if(!thumbnail) return;

    m_deltaMousePos = CCPoint{ pos.x - lastMousePos.x, pos.y - lastMousePos.y };
    m_lastMousePos = pos;

    if(!checkIfInside(popupLayer, pos) && !m_isDragging) return;

    if (m_isTouching) {

        m_isDragging = true;
        m_deltaMousePos = thumbnailPopup->convertToNodeSpace(m_deltaMousePos) ;
       
        CCPoint pos = thumbnail->getPosition();

        CCPoint newPos = pos + m_deltaMousePos;

        float posX = newPos.x;
        float posY = newPos.y;


        thumbnail->setPosition({posX, posY});
    }
    else {
        m_isDragging = false;
    }
}

class $modify(CCScheduler) {
    virtual void update(float dt) {
        Zoom::get()->update(dt);
        CCScheduler::update(dt);
    }
};

class $modify(CCMouseDispatcher) {
    bool dispatchScrollMSG(float y, float x) {
        Zoom::get()->doZoom(y);
        return CCMouseDispatcher::dispatchScrollMSG(y, x);
    }
};

class $modify(CCEGLView) {
    void onGLFWMouseCallBack(GLFWwindow* window, int button, int action, int mods) {
        if (button == GLFW_MOUSE_BUTTON_LEFT) {
            switch(action){
                case GLFW_PRESS: {
                    Zoom::get()->m_isTouching = true;
                    break;
                }
                case GLFW_RELEASE: {
                    Zoom::get()->m_isTouching = false;
                    break;
                }
            }
        }
        CCEGLView::onGLFWMouseCallBack(window, button, action, mods);
    }
};

#endif
