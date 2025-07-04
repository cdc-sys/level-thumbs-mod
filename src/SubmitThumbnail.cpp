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
    if (Mod::get()->hasSavedValue("token")){
        this->step2(Mod::get()->getSavedValue<std::string>("token"));
        return;
    }

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
            auto code = res->code();
            if (code<200||code>299){
                auto error = res->string().unwrapOr(res->errorMessage());
                FLAlertLayer::create("Oops",error,"OK")->show();
                m_loadingOverlay->fadeOut();
                delete this;
                return;
            }
            auto json = res->json().unwrapOrDefault();
            geode::log::info("{} {}",res->code(),json.dump());
            auto token = json["token"].asString().unwrapOrDefault();
            Mod::get()->setSavedValue<std::string>("token", token);
            //auto role = json["user"]["role"].asString().unwrapOrDefault();
            this->step2(token);
        }
    });
    m_eventListener.setFilter(task);
}
void SubmitThumbnail::step2(std::string token){
    m_loadingOverlay->changeStatus("Uploading...");
    std::ifstream ifstream(Mod::get()->getSaveDir()/fmt::format("{}.webp",m_id),std::ios::binary);
    //if (!ifstream.is_open()) return;

    std::vector<uint8_t> data((std::istreambuf_iterator<char>(ifstream)), std::istreambuf_iterator<char>());

    ifstream.close();

    auto req = web::WebRequest();

    m_eventListener.bind([this](web::WebTask::Event* e){
        if (auto res = e->getValue()){
            auto code = res->code();
            if (code<200||code>299){
                auto error = res->string().unwrapOr(res->errorMessage());
                FLAlertLayer::create("Oops",error,"OK")->show();
                m_loadingOverlay->fadeOut();
                if (code == 401){
                    // remove token if unauthorized :D
                    Mod::get()->getSaveContainer().erase("token");
                }
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