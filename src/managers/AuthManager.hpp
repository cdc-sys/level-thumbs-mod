#pragma once
#include <Geode/Geode.hpp>
#include "../utils/AsyncTask.hpp"
#include <argon/argon.hpp>
#include <Geode/utils/web.hpp>
#include "SettingsManager.hpp"
#include "../layers/LoadingOverlay.hpp"

using namespace geode::prelude;


class AuthManager {
private:
    AuthManager() = default;
public:
    AuthManager(AuthManager const&) = delete;
    AuthManager(AuthManager&&) = delete;
    AuthManager& operator=(AuthManager const&) = delete;
    AuthManager& operator=(AuthManager&&) = delete;

    // purely aesthetic using statements tbh
    using LoginTask = Task<geode::Result<std::string>, geode::utils::web::WebProgress>;
    using LinkTask = Task<geode::Result<std::string>, geode::utils::web::WebProgress>;
    using UploadTask = Task<geode::Result<std::string>, geode::utils::web::WebProgress>;

    bool isLoggedIn();
    LoginTask login();
    UploadTask uploadThumbnail(std::string_view filename, int levelID, bool ui=true);
    LinkTask linkAccount(std::string linkSecret);
    
    static std::string_view getToken();

    static AuthManager& get();
};