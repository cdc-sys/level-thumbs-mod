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

    void addBadge(ThumbnailRole role) {
        if (role == ThumbnailRole::NONE) return;

        geode::log::info("badge added: {}",(int)role);

        auto existingBadge = this->getChildByIDRecursive("levelthumbs-badge"_spr);
        if (existingBadge) existingBadge->removeFromParent();

        auto& roleInfo = THUMBNAIL_ROLES[(int)role-1];

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

    void onUpdate(CCObject* sender) {
        ProfilePage::onUpdate(sender);
        AuthManager::get().badgeCache.erase(m_accountID);
    }

    void loadPageFromUserInfo(GJUserScore* score) {
        ProfilePage::loadPageFromUserInfo(score);

        if (!Mod::get()->getSettingValue<bool>("thumb-role-badges")) return;
        
        auto req = web::WebRequest();
        req.userAgent(USER_AGENT);

        auto AM = &AuthManager::get();

        if (AM->badgeCache.contains(m_accountID)) {
            addBadge(AM->badgeCache[m_accountID]);
            return;
        }

        this->m_fields->m_userInfoListener.spawn(
            req.get(fmt::format("{}/user/gd/{}",Settings::thumbnailAPIBaseURL(), this->m_accountID)),
            [this](web::WebResponse res){
                auto AM = &AuthManager::get();
                if (res.ok()){
                    auto json = res.json().unwrapOrDefault();
                    auto role = json["data"]["role"].asString().unwrapOr("user");
                    auto role_enum = getRoleByName(role);
                    addBadge(role_enum);
                    AM->badgeCache[m_accountID] = role_enum;
                } else {
                    AM->badgeCache[m_accountID] = ThumbnailRole::NONE;
                }
            }
        );
    }
};