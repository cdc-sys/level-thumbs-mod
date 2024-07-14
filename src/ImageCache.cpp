
#include <Geode/Geode.hpp>
#include "ImageCache.hpp"


ImageCache* ImageCache::instance = nullptr;

ImageCache::ImageCache(){
    m_imageDict = CCDictionary::create();
}

void ImageCache::addImage(CCImage* image, std::string key){

    if(!image) return;

    if(m_imageDict->count() >= Mod::get()->getSettingValue<int64_t>("cache-limit")){
        m_imageDict->removeObjectForKey(static_cast<CCString*>(m_imageDict->allKeys()->objectAtIndex(0))->getCString());
    }

    m_imageDict->setObject(image, key);
}

CCImage* ImageCache::getImage(std::string key){
    return static_cast<CCImage*>(m_imageDict->objectForKey(key));
    
}