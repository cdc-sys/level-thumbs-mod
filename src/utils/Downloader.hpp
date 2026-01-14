#pragma once
#include <Geode/Geode.hpp>

#define USER_AGENT "LevelThumbnails/" GEODE_PLATFORM_NAME "/" MOD_VERSION

#ifdef GEODE_IS_WINDOWS
#include <wininet.h>
#endif

namespace util {
    struct DownloadProgress {
        size_t downloaded = 0;
        size_t total = 0;

        [[nodiscard]] std::optional<float> downloadProgress() const {
            if (total == 0) {
                return std::nullopt;
            }
            return static_cast<float>(downloaded) / static_cast<float>(total) * 100.f;
        }
    };

    // due to some alien intervention, geode WebRequest seems to sometimes corrupt memory on Windows
    // so as a temporary workaround, we use WinInet directly on Windows.
    // this ends up being faster anyway, because geode::Task adds a lot of bloat
    // TODO: maybe do manual implementation for other platforms too?

    template <class ProgressCallback, class CompleteCallback>
    void downloadFile(
        std::string const& url,
        ProgressCallback&& onProgress,
        CompleteCallback&& onComplete
    ) {
#ifdef GEODE_IS_WINDOWS
        HINTERNET hInternet = InternetOpenA(
            USER_AGENT,
            INTERNET_OPEN_TYPE_PRECONFIG,
            nullptr,
            nullptr,
            0
        );

        if (!hInternet) {
            onComplete(geode::Err("Failed to open internet connection"));
            return;
        }

        HINTERNET hFile = InternetOpenUrlA(
            hInternet,
            url.c_str(),
            nullptr,
            0,
            INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE,
            0
        );

        if (!hFile) {
            InternetCloseHandle(hInternet);
            onComplete(geode::Err("Failed to open URL"));
            return;
        }

        std::vector<uint8_t> buffer;
        constexpr size_t chunkSize = 4096;
        uint8_t tempBuffer[chunkSize];
        DWORD bytesRead;
        DWORD totalBytesRead = 0;

        DWORD contentLength = 0;
        DWORD lengthSize = sizeof(contentLength);
        HttpQueryInfoA(hFile, HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER, &contentLength, &lengthSize, nullptr);

        while (InternetReadFile(hFile, tempBuffer, chunkSize, &bytesRead) && bytesRead > 0) {
            buffer.insert(buffer.end(), tempBuffer, tempBuffer + bytesRead);
            totalBytesRead += bytesRead;

            onProgress({
                totalBytesRead,
                contentLength
            });
        }

        InternetCloseHandle(hFile);
        InternetCloseHandle(hInternet);

        onComplete(geode::Ok(std::move(buffer)));
#else
        geode::utils::web::WebRequest()
            .userAgent(USER_AGENT)
            .get(url)
            .listen(
                [onComplete = std::forward<CompleteCallback>(onComplete)](auto* res) mutable {
                    if (!res->ok()) {
                        switch (res->code()) {
                            default: return onComplete(geode::Err(res->errorMessage()));
                            case 404: return onComplete(geode::Err("Thumbnail not found"));
                            case 500: return onComplete(geode::Err(
                                res->json().unwrapOrDefault()["message"]
                                    .asString().unwrapOr("Internal server error")
                            ));
                        }
                    }

                    onComplete(geode::Ok(res->data()));
                },
                [onProgress = std::forward<ProgressCallback>(onProgress)](auto* prog) mutable {
                    if (!prog) return;
                    onProgress({
                        prog->downloaded(),
                        prog->downloadTotal()
                    });
                }
            );
#endif
    }
}