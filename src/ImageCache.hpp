#pragma once

#ifndef __IMAGECACHE_H
#define __IMAGECACHE_H

#include <Geode/Geode.hpp>
#include "utils.hpp"

using namespace geode::prelude;

class ImageCache {

protected:
    static ImageCache* instance;
private:
    std::string getKey(std::string key, std::string url);
public:

    Ref<CCDictionary> m_imageDict;

    ImageCache();
    void addImage(CCImage* image, std::string key, std::string url);
    CCImage* getImage(std::string key, std::string url);

    static ImageCache* get(){

        if (!instance) {
            instance = new ImageCache();
        };
        return instance;
    }
};


#endif