#include "utils.hpp"
#include <Geode/Geode.hpp>

using namespace geode::prelude;

int levelthumbs::getQualityMultiplier(){
    return 4 / CCDirector::sharedDirector()->getContentScaleFactor();
}

std::string levelthumbs::getBaseUrl(){
    std::string baseURL = Mod::get()->getSettingValue<std::string>("string-setting-example");
	if (baseURL == "")
	    baseURL = "https://raw.githubusercontent.com/cdc-sys/level-thumbnails/main/thumbs";
	if (baseURL.ends_with("/"))
	    baseURL.pop_back(); // remove the last character (aka: as "/")

    return baseURL;
}
