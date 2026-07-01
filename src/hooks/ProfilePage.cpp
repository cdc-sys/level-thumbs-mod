#include <Geode/Geode.hpp>
#include <Geode/modify/ProfilePage.hpp>
#include <Geode/ui/Button.hpp>
#include <Geode/utils/web.hpp>

#include "../managers/AuthManager.hpp"
#include "../managers/SettingsManager.hpp"

using namespace geode::prelude;

class $modify(ProfilePageHook,ProfilePage) {
    struct Fields {
        TaskHolder<web::WebResponse> m_userInfoListener;
    };

    void loadPageFromUserInfo(GJUserScore* score) {
        ProfilePage::loadPageFromUserInfo(score);

        if (!Mod::get()->getSettingValue<bool>("thumb-role-badges")) return;

        auto existingBadge = this->getChildByIDRecursive("levelthumbs-badge"_spr);
        if (existingBadge) existingBadge->removeFromParent();
        
        auto req = web::WebRequest();
        req.userAgent(USER_AGENT);

        this->m_fields->m_userInfoListener.spawn(
            req.get(fmt::format("{}/user/gd/{}",Settings::thumbnailAPIBaseURL(), this->m_accountID)),
            [this](web::WebResponse res){
                if (res.ok()){
                    auto json = res.json().unwrapOrDefault();
                    auto role = json["data"]["role"].asString().unwrapOr("user");

                    auto roleInfoOpt = getRoleInfoByName(role);
                    if (!roleInfoOpt) return;

                    auto& roleInfo = roleInfoOpt.value();
                    auto badgeSprite = CCSprite::createWithSpriteFrameName(roleInfo.badge_sprite.data());
                    // badgeSprite->setAnchorPoint({0.5f,0.55f});
                    auto badgeButton = Button::createWithNode(badgeSprite, [roleInfo](auto sender) {
                        createQuickPopup(
                            roleInfo.name.data(),
                            std::string(roleInfo.description),
                            "OK",
                            nullptr,
                            [](auto, bool) {},
                            true
                        );
                    });

                    badgeButton->setID("levelthumbs-badge"_spr);
                    
                    auto usernameMenu = this->getChildByIDRecursive("username-menu");
                    usernameMenu->addChild(badgeButton);
                    usernameMenu->updateLayout();
                }
            }
        );
    }
};