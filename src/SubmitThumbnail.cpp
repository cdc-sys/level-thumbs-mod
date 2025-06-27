#include "SubmitThumbnail.hpp"
#include "utils.hpp"

SubmitThumbnail::SubmitThumbnail(int id,std::string argonToken,LoadingOverlay* loadingOverlay){
    m_id = id;
    m_argonToken = argonToken;
    m_loadingOverlay = loadingOverlay;   
    this->step1();
    geode::log::info("where");
}

void SubmitThumbnail::step1(){
    auto req = web::WebRequest();

    std::string form = fmt::format("account_id={}&&user_id={}&&username={}&&argon_token={}",
    GJAccountManager::get()->m_accountID,
    (int)GameManager::get()->m_playerUserID,
    GJAccountManager::get()->m_username,
    m_argonToken
    );
    req.bodyString(form);

    auto task =req.post("https://levelthumbs.prevter.me/auth/login");
    m_eventListener.bind([this](web::WebTask::Event* e){
        if (auto res = e->getValue()){
            if (!res->ok()){
                auto error = res->errorMessage();
                FLAlertLayer::create("Oops",error,"OK")->show();
                m_loadingOverlay->fadeOut();
                delete this;
                return;
            }
            auto json = res->json().unwrapOrDefault();
            geode::log::info("{} {}",res->code(),json.dump());
            auto token = json["token"].asString().unwrapOrDefault();
            auto role = json["user"]["role"].asString().unwrapOrDefault();
            this->step2(role,token);
        }
    });
    m_eventListener.setFilter(task);
}
void SubmitThumbnail::step2(std::string role,std::string token){
    std::ifstream ifstream(Mod::get()->getSaveDir().string()+fmt::format("/{}.png",m_id),std::ios::binary);
    //if (!ifstream.is_open()) return;

    std::vector<uint8_t> data((std::istreambuf_iterator<char>(ifstream)), std::istreambuf_iterator<char>());

    ifstream.close();

    auto req = web::WebRequest();

    m_eventListener.bind([this](web::WebTask::Event* e){
        if (auto res = e->getValue()){
            if (res->code()!=201){
                auto error = res->string().unwrapOr(res->errorMessage());
                FLAlertLayer::create("Oops",error,"OK")->show();
                m_loadingOverlay->fadeOut();
                delete this;
                return;
            }
            geode::log::info("{} {}",res->code(),res->string().unwrapOrDefault());
            FLAlertLayer::create("Success!",res->string().unwrapOrDefault(),"OK")->show();
            m_loadingOverlay->fadeOut();
            delete this;
        }
    });

    req.header("Authorization",fmt::format("Bearer {}",token));
    req.body(data); 
    auto task = req.post(fmt::format("https://levelthumbs.prevter.me/upload/{}",m_id));

    m_eventListener.setFilter(task);
}