#pragma once
#include <argon/argon.hpp>
#include <Geode/Geode.hpp>
#include <Geode/utils/web.hpp>
#include "SettingsManager.hpp"
#include "../layers/LoadingOverlay.hpp"

class AuthManager {
private:
    AuthManager() = default;
public:
    AuthManager(AuthManager const&) = delete;
    AuthManager(AuthManager&&) = delete;
    AuthManager& operator=(AuthManager const&) = delete;
    AuthManager& operator=(AuthManager&&) = delete;

    using LoginResult = geode::Result<std::string>;
    using LinkResult = geode::Result<std::string>;
    using UploadResult = geode::Result<std::string>;

    using LoginFuture = arc::Future<LoginResult>;
    using LinkFuture = arc::Future<LinkResult>;
    using UploadFuture = arc::Future<UploadResult>;

    static bool isLoggedIn();
    LoginFuture login();
    UploadFuture uploadThumbnail(std::string_view filename, int levelID, geode::Function<void(geode::ZStringView)> onProgress = nullptr);
    LinkFuture linkAccount(std::string linkSecret);
    
    static std::string_view getToken();

    static AuthManager& get();
};