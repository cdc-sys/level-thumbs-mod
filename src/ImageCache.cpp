
#include <Geode/Geode.hpp>
#include "ImageCache.hpp"


ImageCache* ImageCache::instance = nullptr;

ImageCache::ImageCache(){
    m_imageDict = CCDictionary::create();
}

std::string ImageCache::getKey(std::string key, std::string url){
    return std::format("{}-{}", key, url);
}

void ImageCache::addImage(CCImage* image, std::string key, std::string url){

    if(!image) return;

    if(m_imageDict->count() >= Mod::get()->getSettingValue<int64_t>("cache-limit")){
        m_imageDict->removeObjectForKey(static_cast<CCString*>(m_imageDict->allKeys()->objectAtIndex(0))->getCString());
    }

    m_imageDict->setObject(image, getKey(key, url));
}

CCImage* ImageCache::getImage(std::string key, std::string url){
    return static_cast<CCImage*>(m_imageDict->objectForKey(getKey(key, url)));
}