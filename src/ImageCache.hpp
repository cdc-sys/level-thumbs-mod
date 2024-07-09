#pragma once

#ifndef __IMAGECACHE_H
#define __IMAGECACHE_H

#include <Geode/Geode.hpp>

using namespace geode::prelude;

class ImageCache {

protected:
    static ImageCache* instance;
public:

    Ref<CCDictionary> m_imageDict;

    ImageCache();
    void addImage(CCImage* image, std::string key);
    CCImage* getImage(std::string key);

    static ImageCache* get(){

        if (!instance) {
            instance = new ImageCache();
        };
        return instance;
    }
};


#endif