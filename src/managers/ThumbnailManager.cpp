#include "ThumbnailManager.hpp"
#include "SettingsManager.hpp"

#include <Geode/utils/web.hpp>

// Note: file caching is pretty broken right now
#define DISABLE_FILE_CACHING 1
/*
Setting for file cache limit if we ever want to re-enable it:
"file-cache-limit": {
    "name": "File Cache Limit",
    "description": "The amount of thumbnails that are cached on disk to avoid re-downloading them. -1 for unlimited (may use a lot of disk space).",
    "type": "int",
    "min": -1,
    "default": 150
},
*/

using namespace geode::prelude;

std::string_view format_as(ThumbnailManager::Quality quality) {
    switch (quality) {
        case ThumbnailManager::Quality::Small: return "small";
        case ThumbnailManager::Quality::Medium: return "medium";
        case ThumbnailManager::Quality::High: default: return "high";
    }
}

ThumbnailManager::ThumbnailManager() {
#ifndef DISABLE_FILE_CACHING
    // load file cache
    auto savedCache = Mod::get()->getSavedValue<matjson::Value>("cache");

    auto const readEntry = [](matjson::Value const& val, std::string const& host) -> Result<FileCacheEntry> {
        GEODE_UNWRAP_INTO(int32_t levelID, val["levelID"].asInt());
        GEODE_UNWRAP_INTO(int qualityInt, val["quality"].asInt());
        if (qualityInt < 0 || qualityInt > 2) {
            return Err("Invalid quality in cache");
        }
        auto quality = static_cast<Quality>(qualityInt);
        GEODE_UNWRAP_INTO(uint64_t timestamp, val["lastAccess"].asInt());

        if (!std::filesystem::exists(getThumbnailPath(host, levelID, quality))) {
            return Err("Cached file does not exist");
        }

        return Ok(FileCacheEntry{
            levelID,
            quality,
            host,
            std::chrono::system_clock::time_point(std::chrono::seconds(timestamp)),
        });
    };

    for (auto const& [host, entries] : savedCache) {
        for (auto const& val : entries) {
            auto res = readEntry(val, host);
            if (!res) {
                log::warn("Failed to read thumbnail cache entry: {}", res.unwrapErr());
                continue;
            }
            auto entry = std::move(res).unwrap();
            m_fileCache[fmt::format("{}-{}-{}", entry.levelID, entry.quality, entry.host)] = std::move(entry);
        }
    }

    evictDiskCache();

    // remove stray files
    std::error_code ec;
    for (auto const& dir : std::filesystem::directory_iterator(Mod::get()->getSaveDir() / "cache", ec)) {
        for (auto const& f : std::filesystem::directory_iterator(dir, ec)) {
            bool referenced = std::ranges::any_of(m_fileCache,
                [&](auto const& kv) {
                    return getThumbnailPath(kv.second.host, kv.second.levelID, kv.second.quality) == f.path();
                }
            );
            if (!referenced) {
                std::filesystem::remove(f.path(), ec);
            }
        }
    }
#endif
}

ThumbnailManager& ThumbnailManager::get() {
    static ThumbnailManager instance;
    return instance;
}

ThumbnailManager::FetchTask ThumbnailManager::fetchThumbnail(int32_t levelID, Quality quality) {
    auto key = getThumbnailKey(levelID, quality);

    // check memory cache
    auto it = m_thumbnailCache.find(key);
    if (it != m_thumbnailCache.end()) {
        this->touch(it->first);
        return FetchTask::immediate(Ok(it->second.texture));
    }

    // check file cache
#ifndef DISABLE_FILE_CACHING
    {
        std::unique_lock lock(m_fileCacheMutex);
        auto it2 = m_fileCache.find(key);
        if (it2 != m_fileCache.end()) {
            this->touch(it2->first, true);
            return FetchTask::runWithCallback(
                [this, levelID, quality, key = std::move(key)](auto resolve, auto, auto isCancelled) {
                    auto path = getThumbnailPath(levelID, quality);
                    auto res = file::readBinary(path);
                    if (!res) {
                        // delete the corrupted file from cache
                        log::warn("Failed to read cached thumbnail '{}': {}", path, res.unwrapErr());
                        this->cleanupFileCacheEntry(levelID, quality);
                        resolve(Err("Failed to read cached thumbnail. Retry download"));
                        return;
                    }

                    auto data = std::move(res).unwrap();
                    if (data.empty()) {
                        // delete the corrupted file from cache
                        log::warn("Cached thumbnail is empty");
                        this->cleanupFileCacheEntry(levelID, quality);
                        resolve(Err("Cached thumbnail is empty. Retry download"));
                        return;
                    }

                    this->decodeImageAsync(
                        std::move(data),
                        levelID, quality,
                        std::move(resolve),
                        std::move(isCancelled)
                    );
                },
                "Load Thumbnail from Disk"
            );
        }
    }
#endif

    return FetchTask::runWithCallback(
        [this, levelID, quality, key = std::move(key)](auto resolve, auto progress, auto isCancelled) mutable {
            util::downloadFile(
                getThumbnailUrl(levelID, quality),
                [progress = std::move(progress)](util::DownloadProgress prog) {
                    progress(prog);
                },
                [
                    this, levelID, quality,
                    resolve = std::move(resolve),
                    isCancelled = std::move(isCancelled),
                    key = std::move(key)
                ](Result<ByteVector> res) mutable {
                    if (!res) {
                        resolve(Err(std::move(res).unwrapErr()));
                        return;
                    }

                    if (isCancelled()) {
                        return;
                    }

                    // save to disk cache
                    #ifndef DISABLE_FILE_CACHING
                    if (Settings::thumbnailFileCacheLimit() != 0) {
                        auto path = getThumbnailPath(levelID, quality);
                        auto writeRes = file::writeBinary(path, res.unwrap());
                        if (!writeRes) {
                            log::warn("Failed to write thumbnail {} to disk cache: {}", path, writeRes.unwrapErr());
                        } else {
                            std::unique_lock lock(m_fileCacheMutex);
                            m_fileCache[key] = {
                                levelID,
                                quality,
                                std::string(Settings::thumbnailAPIBaseURL()),
                                std::chrono::system_clock::now(),
                            };
                        }
                    }
                    #endif

                    // WebTask has a really annoying detail, where it copies everything onto main thread upon completion
                    // so this scope is technically main thread right now. Because of that, we spawn a new thread to do the decoding.
                    // On Windows, we use WinInet directly, so we avoid this overhead and just decode on the same thread.
                    // (on why we use WinInet on Windows, read src/utils/Downloader.hpp)
                    // - prevter
                    #ifndef GEODE_IS_WINDOWS
                    std::thread(
                        [this, res = std::move(res), levelID, quality, resolve = std::move(resolve), isCancelled = std::move(isCancelled)]() mutable {
                            this->decodeImageAsync(
                                std::move(res).unwrap(),
                                levelID, quality,
                                std::move(resolve),
                                std::move(isCancelled)
                            );
                        }
                    ).detach();
                    #else
                    this->decodeImageAsync(
                        std::move(res).unwrap(),
                        levelID, quality,
                        std::move(resolve),
                        std::move(isCancelled)
                    );
                    #endif
                }
            );
        },
        "Thumbnail Download Task"
    );
}

ThumbnailManager::CacheState ThumbnailManager::getCacheState(int32_t levelID, Quality quality) {
    auto key = getThumbnailKey(levelID, quality);
    auto it = m_thumbnailCache.find(key);
    if (it != m_thumbnailCache.end()) {
        return CacheState::Ready;
    }

    // check disk cache
#ifndef DISABLE_FILE_CACHING
    std::unique_lock lock(m_fileCacheMutex);
    auto it2 = m_fileCache.find(key);
    if (it2 != m_fileCache.end()) {
        return CacheState::SavedOnDisk;
    }
#endif

    return CacheState::NotCached;
}

std::string ThumbnailManager::getThumbnailUrl(int32_t levelID, Quality quality) {
    if (Settings::isLegacyAPI()) {
        return fmt::format("{}/{}.png", Settings::thumbnailAPIBaseURL(), levelID);
    }
    if (quality == Quality::High) {
        return fmt::format("{}/thumbnail/{}", Settings::thumbnailAPIBaseURL(), levelID);
    }
    return fmt::format("{}/thumbnail/{}/{}", Settings::thumbnailAPIBaseURL(), levelID, quality);
}

#ifndef DISABLE_FILE_CACHING
std::filesystem::path ThumbnailManager::getThumbnailPath(int32_t levelID, Quality quality) {
    // i know legacy api uses png, but tracking separate extensions is harder
    return getCacheDirectory() / fmt::format("{}-{}.webp", levelID, quality);
}

std::filesystem::path ThumbnailManager::getThumbnailPath(std::string_view api, int32_t levelID, Quality quality) {
    return Mod::get()->getSaveDir()
         / "cache"
         / fmt::to_string(std::hash<std::string_view>{}(api))
         / fmt::format("{}-{}.webp", levelID, quality);
}

std::filesystem::path const& ThumbnailManager::getCacheDirectory() {
    static std::filesystem::path path = (
        listenForSettingChanges<std::string>("level-thumbnails-api", [](std::string val) {
            path = Mod::get()->getSaveDir() / "cache" / fmt::to_string(std::hash<std::string_view>{}(val));
        }),
        Mod::get()->getSaveDir() / "cache" / fmt::to_string(std::hash<std::string_view>{}(Settings::thumbnailAPIBaseURL()))
    );

    std::error_code ec;
    std::filesystem::create_directories(path, ec);
    if (ec) {
        log::warn("Failed to create thumbnail cache directory: {}", ec.message());
    }

    return path;
}

void ThumbnailManager::cleanupFileCacheEntry(int32_t levelID, Quality quality) {
    std::unique_lock lock(m_fileCacheMutex);
    auto key = getThumbnailKey(levelID, quality);
    auto it = m_fileCache.find(key);
    if (it != m_fileCache.end()) {
        std::error_code ec;
        std::filesystem::remove(getThumbnailPath(levelID, quality), ec);
        if (ec) {
            log::warn("Failed to remove cached thumbnail {}: {}", getThumbnailPath(levelID, quality), ec.message());
        }
        m_fileCache.erase(it);
    }
}

void ThumbnailManager::saveDiskCache() {
    std::unique_lock lock(m_fileCacheMutex);
    evictDiskCache();
    if (Settings::thumbnailCacheLimit() == 0) {
        return;
    }

    std::unordered_map<std::string, std::vector<matjson::Value>> grouped;
    for (auto const& [key, entry] : m_fileCache) {
        grouped[entry.host].push_back(matjson::makeObject({
            {"levelID", entry.levelID},
            {"quality", static_cast<int>(entry.quality)},
            {"lastAccess", std::chrono::duration_cast<std::chrono::seconds>(entry.lastAccess.time_since_epoch()).count()}
        }));
    }

    Mod::get()->setSavedValue("cache", grouped);
}
#endif

std::string ThumbnailManager::getThumbnailKey(int32_t levelID, Quality quality) {
    return fmt::format("{}-{}-{}", levelID, quality, Settings::thumbnailAPIBaseURL());
}

void ThumbnailManager::decodeImageAsync(
    std::vector<uint8_t> data,
    int32_t levelID, Quality quality,
    FetchTask::PostResult&& resolve,
    FetchTask::HasBeenCancelled&& isCancelled
) {
    if (isCancelled()) {
        return resolve(FetchTask::Cancel{});
    }

    auto img = new CCImage();
    if (!img->initWithImageData(data.data(), data.size())) {
        delete img;
        return resolve(Err("Failed to decode image"));
    }

    if (isCancelled()) {
        delete img;
        return resolve(FetchTask::Cancel{});
    }

    queueInMainThread([this, levelID, quality, resolve = std::move(resolve), img, isCancelled = std::move(isCancelled)]() mutable {
        createTexture(
            img,
            levelID, quality,
            std::move(resolve),
            std::move(isCancelled)
        );
    });
}

void ThumbnailManager::createTexture(
    CCImage* img,
    int32_t levelID, Quality quality,
    FetchTask::PostResult&& resolve,
    FetchTask::HasBeenCancelled&& isCancelled
) {
    // make sure to delete image on exit
    std::unique_ptr<CCImage> imgPtr(img);

    // check cancellation again
    if (isCancelled()) {
        return resolve(FetchTask::Cancel{});
    }

    // create texture
    auto texture = new CCTexture2D();
    if (!texture->initWithImage(img)) {
        delete texture;
        return resolve(Err("Failed to create texture from image"));
    }

    m_thumbnailCache[getThumbnailKey(levelID, quality)] = {
        texture,
        std::chrono::steady_clock::now()
    };
    texture->release();

    resolve(Ok(texture));

    if (!m_scheduledEviction && m_thumbnailCache.size() > Settings::thumbnailCacheLimit()) {
        queueInMainThread([this] { evictIfNeeded(); });
        m_scheduledEviction = true;
    }
}

void ThumbnailManager::touch(std::string const& key, bool fileCache) {
#ifndef DISABLE_FILE_CACHING
    if (fileCache) {
        auto it = m_fileCache.find(key);
        if (it != m_fileCache.end()) {
            it->second.lastAccess = std::chrono::system_clock::now();
        }
    }
#endif

    auto it = m_thumbnailCache.find(key);
    if (it == m_thumbnailCache.end()) return;

    it->second.lastAccess = std::chrono::steady_clock::now();
}

void ThumbnailManager::evictIfNeeded() {
    auto maxCacheSize = Settings::thumbnailCacheLimit();
    if (m_thumbnailCache.size() <= maxCacheSize) {
        return;
    }

    // evict least frequently used
    while (m_thumbnailCache.size() > maxCacheSize) {
        auto it = std::ranges::min_element(
            m_thumbnailCache,
            [](auto const& a, auto const& b) {
                return a.second.lastAccess < b.second.lastAccess; // evict least recently used
            }
        );

        if (it == m_thumbnailCache.end()) break; // should not happen but just in case
        m_thumbnailCache.erase(it);
    }

    m_scheduledEviction = false;
}

#ifndef DISABLE_FILE_CACHING
void ThumbnailManager::evictDiskCache() {
    auto limit = Settings::thumbnailFileCacheLimit();

    // no eviction
    if (limit <= -1) {
        return;
    }

    // delete all files
    if (limit == 0) {
        std::error_code ec;
        std::filesystem::remove_all(getCacheDirectory(), ec);
        if (ec) {
            log::warn("Failed to clear thumbnail disk cache: {}", ec.message());
        } else {
            m_fileCache.clear();
        }
    }

    while (m_fileCache.size() > static_cast<size_t>(limit)) {
        auto it = std::ranges::min_element(
            m_fileCache,
            [](auto const& a, auto const& b) {
                return a.second.lastAccess < b.second.lastAccess;
            }
        );

        if (it == m_fileCache.end()) break;
        std::error_code ec;
        std::filesystem::remove(getThumbnailPath(
            it->second.host,
            it->second.levelID,
            it->second.quality
        ), ec);

        if (ec) {
            log::warn("Failed to remove cached thumbnail file: {}", ec.message());
        }
        m_fileCache.erase(it);
    }
}

$on_mod(DataSaved) {
    ThumbnailManager::get().saveDiskCache();
}
#endif