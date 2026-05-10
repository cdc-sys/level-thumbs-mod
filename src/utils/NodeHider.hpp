#pragma once

struct HideNode {
    CCNode* node = nullptr;
    bool wasVisible = false;

    HideNode(CCNode* parent, std::string_view id) {
        if (parent) {
            node = parent->getChildByID(id);
            if (node) {
                wasVisible = node->isVisible();
                node->setVisible(false);
            }
        }
    }

    HideNode(CCNode* node) {
        if (node) {
            this->node = node;
            wasVisible = node->isVisible();
            node->setVisible(false);
        }
    }

    ~HideNode() {
        if (node) {
            node->setVisible(wasVisible);
        }
    }
};

struct HideGameObject {
    GameObject* node;

    // no idea if we need all of these, but it seems to work
    bool disabled, invisible, visible, hide;
    uint8_t opacity;

    HideGameObject(GameObject* node)
        : node(node),
          disabled(node->m_isDisabled2), invisible(node->m_isInvisible),
          visible(node->isVisible()), hide(node->m_isHide),
          opacity(node->getOpacity())
    {
        node->m_isDisabled2 = true;
        node->m_isInvisible = true;
        node->setOpacity(0);
        node->setVisible(false);
        node->m_isHide = true;
        node->setPosition({-1000, -1000}); // triggers parent batch node invalidation
    }

    ~HideGameObject() {
        node->m_isDisabled2 = disabled;
        node->m_isInvisible = invisible;
        node->setVisible(visible);
        node->m_isHide = hide;
        node->setOpacity(opacity);
    }
};

struct HideCircleWave {
    CCCircleWave* node;
    float opacity, opacityMod;

    HideCircleWave(CCCircleWave* node) : node(node), opacity(node->m_opacity), opacityMod(node->m_opacityMod) {
        node->m_opacity = 0.f;
        node->m_opacityMod = 0.f;
        node->setPosition({-1000, -1000});
    }

    ~HideCircleWave() {
        node->m_opacity = opacity;
        node->m_opacityMod = opacityMod;
    }
};

#define HIDE_NODE(parent, id) HideNode GEODE_CONCAT(hide_, __COUNTER__)(parent, id);
#define HIDE_NODE2(node) HideNode GEODE_CONCAT(hide_, __COUNTER__)(node);