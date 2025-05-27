#include "utils.hpp"

using namespace geode::prelude;

int levelthumbs::getQualityMultiplier(){
    return 4 / CCDirector::sharedDirector()->getContentScaleFactor();
}

std::string levelthumbs::getBaseUrl(){
    bool legacy = Mod::get()->getSettingValue<bool>("legacy-url");
    if (legacy){
        return Mod::get()->getSettingValue<std::string>("level-thumbnails-url");
    } else {
        return Mod::get()->getSettingValue<std::string>("level-thumbnails-url-new");
    }
}