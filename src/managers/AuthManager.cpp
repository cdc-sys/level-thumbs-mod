#include "AuthManager.hpp"
#include "Geode/modify/Modify.hpp"
#include "Geode/ui/Notification.hpp"
#include "Geode/utils/web.hpp"
#include "ThumbnailManager.hpp"
#include <Geode/Result.hpp>
#include <Geode/binding/AccountHelpLayer.hpp>
#include <Geode/binding/GJAccountManager.hpp>
#include <Geode/binding/GJAccountSettingsDelegate.hpp>
#include <Geode/binding/GJAccountSettingsLayer.hpp>
#include <Geode/modify/AccountHelpLayer.hpp>
#include <argon/argon.hpp>
#include <matjson.hpp>
#include <string>
bool AuthManager::isLoggedIn(){
    return Mod::get()->hasSavedValue("token");
}

AuthManager& AuthManager::get() {
    static AuthManager instance;
    return instance;
}

AuthManager::UploadTask AuthManager::uploadThumbnail(std::string_view filename,int levelID,bool ui){
    auto load = LoadingOverlay::create("Logging in...");
    std::string authError = "";
    if (ui) load->show();

    if (!this->isLoggedIn()){
        return AuthManager::login()
        .chain([this,levelID,filename,load](geode::Result<std::string>* res) -> web::WebTask {
            if (!res->isOk()) {
                geode::Notification::create(res->err().value(),NotificationIcon::Error)->show();
                return web::WebTask::immediate(web::WebResponse());
            }
            load->changeStatus("Uploading...");
            auto uploadReq = web::WebRequest();
            auto token = Mod::get()->getSavedValue<std::string>("token");

            std::ifstream ifstream(std::string(filename),std::ios::binary);
            std::vector<uint8_t> data((std::istreambuf_iterator<char>(ifstream)), std::istreambuf_iterator<char>());

            uploadReq.header("Authorization",fmt::format("Bearer {}",token));
            uploadReq.body(data);

            return uploadReq.post(fmt::format("{}/upload/{}",Settings::thumbnailAPIBaseURL(),levelID));
        },"LT Upload Task 1/2")
        .chain([this,load](web::WebResponse* res) -> UploadTask {
            load->fadeOut();
            if (res->code() == 200||res->code() == 200) {
                return UploadTask::immediate(Ok("The thumbnail has been applied."));
            } else if (res->code() == 202) {
                return UploadTask::immediate(Ok("The thumbnail has been submitted, and is now in the queue for approval."));
            } else {
                if (res->code() == 401) Mod::get()->getSaveContainer().erase("token");
                return UploadTask::immediate(Err(fmt::format("Thumbnail upload failed: {}",res->json().unwrapOrDefault()["message"].asString().unwrapOr("auth error"))));
            }

        },"LT Upload Task 2/2");
    }

    load->changeStatus("Uploading...");
    auto uploadReq = web::WebRequest();
    auto token = Mod::get()->getSavedValue<std::string>("token");

    std::ifstream ifstream(std::string(filename),std::ios::binary);
    std::vector<uint8_t> data((std::istreambuf_iterator<char>(ifstream)), std::istreambuf_iterator<char>());

    uploadReq.header("Authorization",fmt::format("Bearer {}",token));
    uploadReq.body(data);

    return uploadReq.post(fmt::format("{}/upload/{}",Settings::thumbnailAPIBaseURL(),levelID))
        .chain([this,load](web::WebResponse* res) -> UploadTask {
            load->fadeOut();
            if (res->code() == 201||res->code() == 200) {
                return UploadTask::immediate(Ok("The thumbnail has been applied."));
            } else if (res->code() == 202) {
                return UploadTask::immediate(Ok("The thumbnail has been submitted, and is now in the queue for approval."));
            } else {
                if (res->code() == 401) Mod::get()->getSaveContainer().erase("token");
                geode::log::error("{}",res->string().unwrapOr(""));
                return UploadTask::immediate(Err(fmt::format("Thumbnail upload failed: {}",res->json().unwrapOrDefault()["message"].asString().unwrapOr("auth error"))));
            }

        },"LT Upload Task 2/2");
}

AuthManager::LinkTask AuthManager::linkAccount(std::string linkSecret){
   
    geode::log::info("{}",linkSecret);
     auto load = LoadingOverlay::create("Linking..");
     load->show();

    if (!this->isLoggedIn()){
        return AuthManager::login()
        .chain([this,linkSecret](geode::Result<std::string>* res) -> web::WebTask {
            if (!res->isOk()) {
                geode::Notification::create(res->err().value(),NotificationIcon::Error)->show();
                return web::WebTask::immediate(web::WebResponse());
            }
            auto linkReq = web::WebRequest();
            auto token = Mod::get()->getSavedValue<std::string>("token");

            linkReq.header("Authorization",fmt::format("Bearer {}",token));
            auto body = matjson::makeObject({{"token",linkSecret}});
            linkReq.bodyJSON(body);

            return linkReq.post(fmt::format("{}/auth/link",Settings::thumbnailAPIBaseURL()));
        },"LT Link Task 1/2")
        .chain([load](web::WebResponse* res) -> LinkTask {
            load->fadeOut();
            if (res->ok()){
                auto token = res->json().unwrapOrDefault()["token"].asString().unwrapOrDefault();
                Mod::get()->setSavedValue<std::string>("token", token);
                return LinkTask::immediate(Ok("Your account was linked successfully."));
            } else {
                return LinkTask::immediate(Err(fmt::format("Account link failed: {}",res->json().unwrapOrDefault()["message"].asString().unwrapOr("auth error"))));
            }

        },"LT Link Task 2/2");

    } else {
        auto linkReq = web::WebRequest();
        auto token = Mod::get()->getSavedValue<std::string>("token");

        linkReq.header("Authorization",fmt::format("Bearer {}",token));
        auto body = matjson::makeObject({{"token",linkSecret}});
        linkReq.bodyJSON(body);

        return linkReq.post(fmt::format("{}/auth/link",Settings::thumbnailAPIBaseURL()))
        .chain([load](web::WebResponse* res) -> LinkTask {
            load->fadeOut();
            if (res->ok()){
                auto token = res->json().unwrapOrDefault()["token"].asString().unwrapOrDefault();
                Mod::get()->setSavedValue<std::string>("token", token);
                return LinkTask::immediate(Ok("Your account was linked successfully."));
            } else {
                return LinkTask::immediate(Err(fmt::format("Account link failed: {}",res->json().unwrapOrDefault()["message"].asString().unwrapOr("auth error"))));
            }

        },"LT Link Task 2/2");
    }
}

AuthManager::LoginTask AuthManager::login(){
    if (GJAccountManager::get()->m_accountID == 0) return LoginTask::immediate(Err("not logged into an account"));
    auto data = argon::getGameAccountData();
    auto accID = GJAccountManager::get()->m_accountID;
    auto userID = GameManager::get()->m_playerUserID;
    auto username = GJAccountManager::get()->m_username;

    return argon::startAuthWithAccount(data,false)
        .chain([this, accID,userID = userID.value(), username = std::move(username)](geode::Result<std::string>* res) -> web::WebTask {
            if (!res->isOk()) {
                geode::Notification::create(res->err().value(),NotificationIcon::Error)->show();
                return web::WebTask::immediate(web::WebResponse());
            }

            auto token = res->unwrapOr("");
            auto loginReq = web::WebRequest();
            auto body = matjson::makeObject({{"account_id",accID},{"user_id",userID},{"username",std::string(username)},{"argon_token",token}});
            
            loginReq.bodyJSON(body);

            return loginReq.post(fmt::format("{}/auth/login",Settings::thumbnailAPIBaseURL()));
        },"LT Login Task 1/2")
        .chain([this](web::WebResponse* res) -> LoginTask {
            if (res->ok()){
                auto obj = res->json().unwrapOrDefault();
                auto token = obj["token"].asString().unwrapOr("");

                Mod::get()->setSavedValue<std::string>("token",token);

                return LoginTask::immediate(Ok("success!"));

            } else {
                return LoginTask::immediate(Err("login request failed"));
            }

        },"LT Login Task 2/2");
}

class $modify(AccountHelpLayer){
    void FLAlert_Clicked(FLAlertLayer* p0, bool p1) {
        if (p0->getTag()==4&&p1){
            Mod::get()->getSaveContainer().erase("token");
        }
        AccountHelpLayer::FLAlert_Clicked(p0, p1);
    }
};