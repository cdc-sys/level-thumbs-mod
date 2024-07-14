#pragma once

#ifndef __ZOOM_H
#define __ZOOM_H

#include <Geode/Geode.hpp>

using namespace geode::prelude;

class Zoom {

protected:
    static Zoom* instance;
    CCPoint m_lastMousePos;
    CCPoint m_deltaMousePos;
    bool m_isDragging = false;
public:
    bool m_isTouching = false;

    void onScroll(float x, float y);
    void update(float dt);
    void doZoom(float zoom);

    static Zoom* get(){

        if (!instance) {
            instance = new Zoom();
        };
        return instance;
    }
};


#endif