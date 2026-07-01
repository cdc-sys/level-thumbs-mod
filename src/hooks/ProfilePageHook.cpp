#include <Geode/modify/ProfilePage.hpp>
#include <Geode/utils/web.hpp>
#include "../managers/SettingsManager.hpp"
#include "../managers/AuthManager.hpp"
#include <Geode/ui/Button.hpp>
#include <Geode/Geode.hpp>

using namespace geode::prelude;

class $modify(ProfilePageHook,ProfilePage) {
    struct Fields {
        TaskHolder<web::WebResponse> m_userInfoListener;
    };
    void loadPageFromUserInfo(GJUserScore* score) {
        ProfilePage::loadPageFromUserInfo(score);

        if (!Mod::get()->getSettingValue<bool>("thumb-role-badges")) return;
        
        auto req = web::WebRequest();
        this->m_fields->m_userInfoListener.spawn(
            req.get(fmt::format("{}/user/gd/{}",Settings::thumbnailAPIBaseURL(),this->m_accountID)),
            [this](web::WebResponse res){
                if (res.ok()){
                    auto json = res.json().unwrapOrDefault();
                    auto role = json["data"]["role"].asString().unwrapOr("user");
                    auto roleInfo = getRoleInfoByName(role);
                    auto badgeSprite = CCSprite::create(roleInfo.badge_sprite.c_str());
                    badgeSprite->setScale(0.075f);
                    //badgeSprite->setAnchorPoint({0.5f,0.55f});
                    auto badgeButton = geode::Button::createWithNode(badgeSprite, [roleInfo = std::move(roleInfo)](auto sender){
                        createQuickPopup(roleInfo.name.c_str(),roleInfo.description,"OK",nullptr,[](auto,bool){},true);
                        return;
                    });
                    auto usernameMenu = this->getChildByIDRecursive("username-menu");
                    usernameMenu->addChild(badgeButton);
                    usernameMenu->updateLayout();

                } else {
                    geode::log::info("No info i guess");
                }
            }
        );
    }
};