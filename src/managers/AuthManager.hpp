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
    AuthManager(){}
public:
    AuthManager(AuthManager const&) = delete;
    AuthManager(AuthManager&&) = delete;
    AuthManager& operator=(AuthManager const&) = delete;
    AuthManager& operator=(AuthManager&&) = delete;

    using LoginTask = Task<geode::Result<std::string>, geode::utils::web::WebProgress>;
    using UploadTask = Task<geode::Result<std::string>, geode::utils::web::WebProgress>;
    using AuthLoginTask = Task<geode::Result<std::string>, argon::AuthProgress>;
    AuthLoginTask getAuthTask(argon::AccountData account, bool forceStrong);

    bool isLoggedIn();
    LoginTask login();
    UploadTask uploadThumbnail(std::string_view filname,int levelID,bool ui=true);
    
    static std::string_view getToken();

    static AuthManager& get();
};