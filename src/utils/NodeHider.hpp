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

#define HIDE_NODE(parent, id) HideNode GEODE_CONCAT(hide_, __COUNTER__)(parent, id);
#define HIDE_NODE2(node) HideNode GEODE_CONCAT(hide_, __COUNTER__)(node);