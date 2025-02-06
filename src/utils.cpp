#include "utils.hpp"
#include <Geode/Geode.hpp>

using namespace geode::prelude;

int levelthumbs::getQualityMultiplier(){
    return 4 / CCDirector::sharedDirector()->getContentScaleFactor();
}

std::string levelthumbs::getBaseUrl(){
    std::string baseURL = Mod::get()->getSettingValue<std::string>("level-thumbnails-url");
    return baseURL;
}
