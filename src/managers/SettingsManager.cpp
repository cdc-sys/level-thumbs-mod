#include "SettingsManager.hpp"

using namespace geode::prelude;

bool Settings::showInBrowser() {
    static bool value = (
        listenForSettingChanges<bool>("show-in-browser",[](bool val) { value = val; }),
        getMod()->getSettingValue<bool>("show-in-browser")
    );
    return value;
}

bool Settings::showThumbnailButton() {
    static bool value = (
        listenForSettingChanges<bool>("thumbnailButton",[](bool val) { value = val; }),
        getMod()->getSettingValue<bool>("thumbnailButton")
    );
    return value;
}

bool Settings::listsLimitEnabled() {
    static bool value = (
        listenForSettingChanges<bool>("lists-levels-limit-enabled",[](bool val) { value = val; }),
        getMod()->getSettingValue<bool>("lists-levels-limit-enabled")
    );
    return value;
}

int64_t Settings::listsLevelsLimit() {
    static int64_t value = (
        listenForSettingChanges<int64_t>("lists-levels-limit",[](int64_t val) { value = val; }),
        getMod()->getSettingValue<int64_t>("lists-levels-limit")
    );
    return value;
}

bool Settings::thumbnailTakingEnabled() {
    static bool value = (
        listenForSettingChanges<bool>("enable-thumbnail-taking",[](bool val) { value = val; }),
        getMod()->getSettingValue<bool>("enable-thumbnail-taking")
    );
    return value;
}

int64_t Settings::thumbnailCacheLimit() {
    static int64_t value = (
        listenForSettingChanges<int64_t>("cache-limit",[](int64_t val) { value = val; }),
        getMod()->getSettingValue<int64_t>("cache-limit")
    );
    return value;
}

int64_t Settings::thumbnailFileCacheLimit() {
    static int64_t value = (
        listenForSettingChanges<int64_t>("file-cache-limit",[](int64_t val) { value = val; }),
        getMod()->getSettingValue<int64_t>("file-cache-limit")
    );
    return value;
}

std::string_view Settings::thumbnailAPIBaseURL() {
    static std::string value = (
        listenForSettingChanges<std::string>("level-thumbnails-api",[](std::string val) { value = std::move(val); }),
        getMod()->getSettingValue<std::string>("level-thumbnails-api")
    );
    return value;
}

bool Settings::isLegacyAPI() {
    static bool value = (
        listenForSettingChanges<bool>("legacy-url",[](bool val) { value = val; }),
        getMod()->getSettingValue<bool>("legacy-url")
    );
    return value;
}

bool Settings::isShowLevelBackground() {
    static bool value = (
        listenForSettingChanges<bool>("level-bg",[](bool val) { value = val; }),
        getMod()->getSettingValue<bool>("level-bg")
    );
    return value;
}

uint8_t Settings::backgroundDimAmount() {
    static uint8_t value = (
        listenForSettingChanges<int64_t>("darkening",[](int64_t val) { value = static_cast<uint8_t>(val); }),
        static_cast<uint8_t>(getMod()->getSettingValue<int64_t>("darkening"))
    );
    return value;
}

bool Settings::isBackgroundBlurred() {
    static bool value = (
        listenForSettingChanges<bool>("level-bg-blur",[](bool val) { value = val; }),
        getMod()->getSettingValue<bool>("level-bg-blur")
    );
    return value;
}

int64_t Settings::backgroundBlurRadius() {
    static int64_t value = (
        listenForSettingChanges<int64_t>("blur-strength",[](int64_t val) { value = val; }),
        getMod()->getSettingValue<int64_t>("blur-strength")
    );
    return value;
}

static ThumbnailManager::Quality convertFromString(std::string_view str) {
    if (str == "High") return ThumbnailManager::Quality::High;
    if (str == "Medium") return ThumbnailManager::Quality::Medium;
    if (str == "Low") return ThumbnailManager::Quality::Small;
    return ThumbnailManager::Quality::High;
}

ThumbnailManager::Quality Settings::backgroundQuality() {
    static ThumbnailManager::Quality value = (
        listenForSettingChanges<std::string>("level-bg-quality",[](std::string val) {
            value = convertFromString(val);
        }),
        convertFromString(getMod()->getSettingValue<std::string>("level-bg-quality"))
    );
    return value;
}