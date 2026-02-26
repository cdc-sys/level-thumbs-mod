#include "AuthManager.hpp"
#include "ThumbnailManager.hpp"

#include <matjson.hpp>
#include <string>
#include <argon/argon.hpp>
#include <Geode/modify/AccountHelpLayer.hpp>
#include <Geode/ui/Notification.hpp>
#include <Geode/utils/web.hpp>

using namespace geode::prelude;

bool AuthManager::isLoggedIn() {
    return Mod::get()->hasSavedValue("token");
}

AuthManager& AuthManager::get() {
    static AuthManager instance;
    return instance;
}

AuthManager::UploadFuture AuthManager::uploadThumbnail(std::string_view filename, int levelID, Function<void(ZStringView)> onProgress) {
    if (onProgress) onProgress("Logging in...");

    if (!this->isLoggedIn()) {
        auto loginRes = co_await this->login();
        if (!loginRes) {
            co_return Err(std::move(loginRes).unwrapErr());
        }
    }

    if (onProgress) onProgress("Uploading...");

    auto readRes = file::readBinary(filename);
    if (!readRes) {
        co_return Err(std::move(readRes).unwrapErr());
    }

    auto res = co_await web::WebRequest()
        .header("Authorization", fmt::format("Bearer {}", Mod::get()->getSavedValue<std::string>("token")))
        .body(std::move(readRes).unwrap())
        .userAgent(USER_AGENT)
        .post(fmt::format("{}/upload/{}", Settings::thumbnailAPIBaseURL(), levelID));

    auto code = res.code();

    if (code == 201 || code == 200) {
        co_return Ok("The thumbnail has been applied.");
    } else if (code == 202) {
        co_return Ok("The thumbnail has been submitted, and is now in the queue for approval.");
    }

    if (code == 401) Mod::get()->getSaveContainer().erase("token");
    log::error("{}", res.string().unwrapOrDefault());
    co_return Err(fmt::format(
        "Thumbnail upload failed: {}",
        res.json().unwrapOrDefault()["message"].asString().unwrapOr("auth error")
    ));
}

AuthManager::LinkFuture AuthManager::linkAccount(std::string linkSecret) {
    if (!this->isLoggedIn()){
        auto loginRes = co_await this->login();
        if (!loginRes) {
            co_return Err(std::move(loginRes).unwrapErr());
        }
    }

    auto res = co_await web::WebRequest()
        .header("Authorization", fmt::format("Bearer {}", Mod::get()->getSavedValue<std::string>("token")))
        .bodyJSON(matjson::makeObject({{"token", std::move(linkSecret)}}))
        .userAgent(USER_AGENT)
        .post(fmt::format("{}/auth/link", Settings::thumbnailAPIBaseURL()));

    if (res.ok()) {
        Mod::get()->setSavedValue<std::string>(
            "token", res.json().unwrapOrDefault()["token"].asString().unwrapOrDefault()
        );
        co_return Ok("Your account was linked successfully.");
    }

    co_return Err(fmt::format(
        "Account link failed: {}",
        res.json().unwrapOrDefault()["message"].asString().unwrapOr("auth error")
    ));
}

AuthManager::LoginFuture AuthManager::login() {
    if (GJAccountManager::get()->m_accountID == 0) {
        co_return Err("not logged into an account");
    }

    auto argonRes = co_await argon::startAuth();
    if (!argonRes) {
        co_return Err(std::move(argonRes).unwrapErr());
    }

    auto accID = GJAccountManager::get()->m_accountID;
    auto userID = GameManager::get()->m_playerUserID.value();
    std::string username = GJAccountManager::get()->m_username;

    auto token = std::move(argonRes).unwrap();
    web::WebResponse res = co_await web::WebRequest()
        .bodyJSON(matjson::makeObject({
            {"account_id", accID},
            {"user_id", userID},
            {"username", std::move(username)},
            {"argon_token", std::move(token)}
        }))
        .userAgent(USER_AGENT)
        .post(fmt::format("{}/auth/login", Settings::thumbnailAPIBaseURL()));

    if (!res.ok()) {
        log::error("Login request failed: {}", res.errorMessage());
        co_return Err("login request failed");
    }

    Mod::get()->setSavedValue<std::string>(
        "token", res.json().unwrapOrDefault()["token"].asString().unwrapOrDefault()
    );

    co_return Ok("success!");
}

class $modify(AccountHelpLayer) {
    void FLAlert_Clicked(FLAlertLayer* p0, bool p1) override {
        if (p0->getTag() == 4 && p1){
            Mod::get()->getSaveContainer().erase("token");
        }
        AccountHelpLayer::FLAlert_Clicked(p0, p1);
    }
};