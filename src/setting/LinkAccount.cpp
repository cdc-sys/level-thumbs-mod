#include <utility>
#include <Geode/loader/Mod.hpp>
#include <Geode/loader/SettingV3.hpp>
#include <Geode/ui/Layout.hpp>
#include <Geode/ui/TextInput.hpp>
#include <Geode/utils/cocos.hpp>
#include "../layers/ConfirmAlertLayer.hpp"
#include "../managers/AuthManager.hpp"

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

class MyCustomSettingV3 : public SettingBaseValueV3<MyCustomEnum> {
protected:
    bool m_splorgy = false;

public:
    static Result<std::shared_ptr<SettingV3>> parse(
        std::string const& key,
        std::string const& modID,
        matjson::Value const& json
    ) {
        auto res = std::make_shared<MyCustomSettingV3>();
        auto root = checkJson(json, "MyCustomSettingV3");
        res->parseBaseProperties(key, modID, root);
        root.has("splorgy").into(res->m_splorgy);
        root.checkUnknownKeys();
        return root.ok(std::static_pointer_cast<SettingV3>(res));
    }

    SettingNodeV3* createNode(float width) override;
};

template <>
struct geode::SettingTypeForValueType<MyCustomEnum> {
    using SettingType = MyCustomSettingV3;
};

class MyCustomSettingNodeV3 : public SettingValueNodeV3<MyCustomSettingV3> {
protected:
    TextInput* m_linkTokenInput = nullptr;
    TaskHolder<AuthManager::LinkResult> m_linkListener;

    bool init(std::shared_ptr<MyCustomSettingV3> setting, float width) {
        if (!SettingValueNodeV3::init(std::move(setting), width))
            return false;

        auto linkButton = CCMenuItemExt::createSpriteExtra(
            ButtonSprite::create("Link"),
            [this](auto self){
                ConfirmAlertLayer::create(
                    [this](bool btn2){
                        if (btn2) {
                            auto load = LoadingOverlay::create("Linking..");
                            load->show();
                            m_linkListener.spawn(
                                AuthManager::get().linkAccount(m_linkTokenInput->getString()),
                                [load](auto res){
                                    load->fadeOut();
                                    if (res.isOk()) {
                                        FLAlertLayer::create("Success!", res.unwrapOrDefault(), "OK")->show();
                                    } else {
                                        FLAlertLayer::create("Error!", fmt::format("<cr>{}</c>", res.unwrapErr()), "OK")->show();
                                    }
                                }
                            );
                        }
                    },
                    "Warning!",
                    "This process is <cr>irreversible</c> and will link your accounts <cy>forever</c>!\nAre you sure you want to proceed?",
                    "No", "Yes"
                )->show();
            }
        );
        m_linkTokenInput = TextInput::create(150, "Link secret...", "bigFont.fnt");
        m_linkTokenInput->setFilter("QWERTYUIOPASDFGHJKLZXCVBNMqwertyuiopasdfghjklzxcvbnm1234567890_-.");

        this->getButtonMenu()->addChild(m_linkTokenInput);
        this->getButtonMenu()->addChild(linkButton);

        auto layout = AxisLayout::create(Axis::Row);
        layout->setAxisReverse(false);
        layout->setAutoGrowAxis(1.f);
        layout->setGap(10.f);
        this->getButtonMenu()->setLayout(layout);
        this->getButtonMenu()->updateLayout();
        this->getButtonMenu()->setScale(0.7f);

        this->updateState(nullptr);

        return true;
    }

    void updateState(CCNode* invoker) override {
        SettingValueNodeV3::updateState(invoker);
    }

    void onToggle(CCObject* sender) {}

public:
    static MyCustomSettingNodeV3* create(std::shared_ptr<MyCustomSettingV3> setting, float width) {
        auto ret = new MyCustomSettingNodeV3();
        if (ret->init(std::move(setting), width)) {
            ret->autorelease();
            return ret;
        }
        delete ret;
        return nullptr;
    }
};

SettingNodeV3* MyCustomSettingV3::createNode(float width) {
    return MyCustomSettingNodeV3::create(
        std::static_pointer_cast<MyCustomSettingV3>(shared_from_this()),
        width
    );
}

$execute {
    (void)Mod::get()->registerCustomSettingType("link-account", &MyCustomSettingV3::parse);
}