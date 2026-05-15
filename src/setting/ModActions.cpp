#include <utility>
#include <Geode/loader/Mod.hpp>
#include <Geode/loader/SettingV3.hpp>
#include <Geode/ui/Layout.hpp>
#include <Geode/ui/TextInput.hpp>
#include <Geode/utils/cocos.hpp>
#include "../managers/ThumbnailManager.hpp"

using namespace geode::prelude;

enum class MyCustomEnum {
    ValidEnumValue,
    OtherValidEnumValue,
};

template <>
struct matjson::Serialize<MyCustomEnum> {
    static Result<MyCustomEnum> fromJson(const Value& value) {
        return Ok(MyCustomEnum{});
    }
    static Value toJson(const MyCustomEnum& user) {
        return makeObject({});
    }
};

class ModActionSetting : public SettingBaseValueV3<MyCustomEnum> {
protected:
    bool m_splorgy = false;

public:
    static Result<std::shared_ptr<SettingV3>> parse(
        std::string const& key,
        std::string const& modID,
        matjson::Value const& json
    ) {
        auto res = std::make_shared<ModActionSetting>();
        auto root = checkJson(json, "ModActionSetting");
        res->parseBaseProperties(key, modID, root);
        root.has("splorgy").into(res->m_splorgy);
        root.checkUnknownKeys();
        return root.ok(std::static_pointer_cast<SettingV3>(res));
    }

    SettingNodeV3* createNode(float width) override;
};

template <>
struct geode::SettingTypeForValueType<MyCustomEnum> {
    using SettingType = ModActionSetting;
};

class ModActionSettingNode : public SettingValueNodeV3<ModActionSetting> {
protected:

    bool init(std::shared_ptr<ModActionSetting> setting, float width) {
        if (!SettingValueNodeV3::init(std::move(setting), width))
            return false;

        auto cacheButtonSpr = ButtonSprite::create("Clear Cache");
        auto cacheButton = CCMenuItemExt::createSpriteExtra(cacheButtonSpr, [](auto){
            std::filesystem::remove_all(ThumbnailManager::get().getCacheDirectory());
            geode::Notification::create("Cache Cleared",geode::NotificationIcon::Success)->show();
        });
        auto logoutButtonSpr = ButtonSprite::create("Log Out");
        auto logoutButton = CCMenuItemExt::createSpriteExtra(logoutButtonSpr, [this](auto button){
            Mod::get()->getSaveContainer().erase("token");
            button->removeFromParent();
            this->getButtonMenu()->updateLayout();
            geode::Notification::create("Logged Out",geode::NotificationIcon::Success)->show();
        });

        this->getButtonMenu()->addChild(cacheButton);
        if (Mod::get()->getSaveContainer().contains("token")) this->getButtonMenu()->addChild(logoutButton);
        this->getNameLabel()->setVisible(false);
        this->getDescriptionButton()->setVisible(false);

        auto layout = AxisLayout::create(Axis::Row);
        layout->setAxisReverse(false);
        layout->setAutoGrowAxis(1.f);
        layout->setGap(10.f);
        this->getButtonMenu()->setLayout(layout);
        this->getButtonMenu()->updateLayout();
        this->getButtonMenu()->setScale(0.7f);
        this->getButtonMenu()->setAnchorPoint({0.5f,0.5f});
        this->getButtonMenu()->setPositionX(width/2);

        this->updateState(nullptr);

        return true;
    }

    void updateState(CCNode* invoker) override {
        SettingValueNodeV3::updateState(invoker);
    }

    void onToggle(CCObject* sender) {}

public:
    static ModActionSettingNode* create(std::shared_ptr<ModActionSetting> setting, float width) {
        auto ret = new ModActionSettingNode();
        if (ret->init(std::move(setting), width)) {
            ret->autorelease();
            return ret;
        }
        delete ret;
        return nullptr;
    }
};

SettingNodeV3* ModActionSetting::createNode(float width) {
    return ModActionSettingNode::create(
        std::static_pointer_cast<ModActionSetting>(shared_from_this()),
        width
    );
}

$execute {
    (void)Mod::get()->registerCustomSettingType("mod-actions", &ModActionSetting::parse);
}