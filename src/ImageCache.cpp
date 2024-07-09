
#include <Geode/Geode.hpp>
#include "ImageCache.hpp"


ImageCache* ImageCache::instance = nullptr;

ImageCache::ImageCache(){
    m_imageDict = CCDictionary::create();
}

void ImageCache::addImage(CCImage* image, std::string key){

    if(m_imageDict->count() >= Mod::get()->getSettingValue<int64_t>("cache-limit")){
        m_imageDict->removeObjectForKey(m_imageDict->getFirstKey());
    }

    m_imageDict->setObject(image, key);
}

CCImage* ImageCache::getImage(std::string key){
    return static_cast<CCImage*>(m_imageDict->objectForKey(key));
    
}