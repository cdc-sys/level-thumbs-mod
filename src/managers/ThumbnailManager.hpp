#pragma once
#include <Geode/Geode.hpp>
#include "../utils/AsyncTask.hpp"

class ThumbnailManager {
private:
    ThumbnailManager();

public:
    ThumbnailManager(ThumbnailManager const&) = delete;
    ThumbnailManager(ThumbnailManager&&) = delete;
    ThumbnailManager& operator=(ThumbnailManager const&) = delete;
    ThumbnailManager& operator=(ThumbnailManager&&) = delete;

    static ThumbnailManager& get();

    enum class Quality {
        Small,
        Medium,
        High
    };

    enum class CacheState {
        NotCached,
        SavedOnDisk,
        Ready
    };

    using FetchTask = CustomTask<geode::Result<geode::Ref<cocos2d::CCTexture2D>>, geode::utils::web::WebProgress>;
    FetchTask fetchThumbnail(int32_t levelID, Quality quality = Quality::High);
    CacheState getCacheState(int32_t levelID, Quality quality = Quality::High);

    static std::string getThumbnailUrl(int32_t levelID, Quality quality = Quality::High);
    static std::filesystem::path getThumbnailPath(int32_t levelID, Quality quality = Quality::High);
    static std::filesystem::path getThumbnailPath(std::string_view api, int32_t levelID, Quality quality);

    static std::filesystem::path const& getCacheDirectory();

    void cleanupFileCacheEntry(int32_t levelID, Quality quality);

    void saveDiskCache();

private:
    static std::string getThumbnailKey(int32_t levelID, Quality quality);

    void decodeImageAsync(
        std::vector<uint8_t> data,
        int32_t levelID, Quality quality,
        FetchTask::PostResult&& resolve,
        FetchTask::HasBeenCancelled&& isCancelled
    );

    void createTexture(
        std::unique_ptr<uint8_t[]> data,
        uint16_t width, uint16_t height, bool hasAlpha,
        int32_t levelID, Quality quality,
        FetchTask::PostResult&& resolve,
        FetchTask::HasBeenCancelled&& isCancelled
    );

    void touch(std::string const& key, bool fileCache = false);
    void evictIfNeeded();
    void evictDiskCache();

    struct CacheEntry {
        geode::Ref<cocos2d::CCTexture2D> texture;
        std::chrono::steady_clock::time_point lastAccess;
    };

    struct FileCacheEntry {
        int32_t levelID;
        Quality quality;
        std::string host;
        std::chrono::system_clock::time_point lastAccess;
    };

private:
    std::unordered_map<std::string, CacheEntry> m_thumbnailCache;
    std::unordered_map<std::string, FileCacheEntry> m_fileCache;
    std::mutex m_cacheMutex;
    std::mutex m_fileCacheMutex;
    bool m_scheduledEviction = false;
};