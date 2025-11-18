#include "AuthManager.hpp"
#include "Geode/utils/web.hpp"
#include "ThumbnailManager.hpp"
#include <Geode/Result.hpp>
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
    if (ui) load->show();
    if (!this->isLoggedIn()){
        return AuthManager::login()
        .chain([this,levelID,filename,load](geode::Result<std::string>* res) -> web::WebTask {
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
                geode::log::error("{}",res->errorMessage());
                return UploadTask::immediate(Err(fmt::format("Thumbnail upload failed: {}",res->string().unwrapOr(res->errorMessage()))));
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
                geode::log::error("{}",res->string().unwrapOr(""));
                return UploadTask::immediate(Err(fmt::format("Thumbnail upload failed: {}",res->string().unwrapOr(res->errorMessage()))));
            }

        },"LT Upload Task 2/2");
}

AuthManager::LoginTask AuthManager::login(){
    auto data = argon::getGameAccountData();
    auto accID = GJAccountManager::get()->m_accountID;
    auto userID = GameManager::get()->m_playerUserID;
    auto username = GJAccountManager::get()->m_username;
    return argon::startAuthWithAccount(data,false)
        .chain([this, accID,userID = userID.value(), username = std::move(username)](geode::Result<std::string>* res) -> web::WebTask {
            if (!res->isOk()){  
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