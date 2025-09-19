#pragma once
#include "ThumbnailManager.hpp"

class Settings {
public:
    static bool showInBrowser();
    static bool showThumbnailButton();
    static bool thumbnailTakingEnabled();

    static int64_t thumbnailCacheLimit();
    static int64_t thumbnailFileCacheLimit();

    static std::string_view thumbnailAPIBaseURL();
    static bool isLegacyAPI();

    static bool isShowLevelBackground();
    static ThumbnailManager::Quality backgroundQuality();
    static uint8_t backgroundDimAmount();
    static bool isBackgroundBlurred();
    static int64_t backgroundBlurRadius();
};