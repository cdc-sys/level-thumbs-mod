#include "Geode/ui/Layout.hpp"
#include "Geode/ui/TextInput.hpp"
#include "Geode/utils/cocos.hpp"
#include <Geode/loader/SettingV3.hpp>
#include <Geode/loader/Mod.hpp>
#include "../managers/AuthManager.hpp"
#include "../layers/ConfirmAlertLayer.hpp"

using namespace geode::prelude;


enum class MyCustomEnum {
    ValidEnumValue,
    OtherValidEnumValue,
};


template <>
struct matjson::Serialize<MyCustomEnum> {
    static geode::Result<MyCustomEnum> fromJson(const matjson::Value& value) {
        return geode::Ok(MyCustomEnum{});
    }
    static matjson::Value toJson(const MyCustomEnum& user) {
        return matjson::makeObject({});
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
    geode::TextInput* m_linkTokenInput;
    EventListener<AuthManager::LinkTask> m_linkListener;

    void handleLinking(AuthManager::LinkTask::Event* event){
        if (auto res = event->getValue()) {
            // run on next frame to prevent a race condition from happening and breaking the entire touch system (??????????????/
            if (res->isOk()) {
                FLAlertLayer::create("Success!",res->unwrapOrDefault(),"OK")->show();
            } else {
                FLAlertLayer::create("Error!",fmt::format("<cr>{}</c>",res->err().value_or("")),"OK")->show();
            }
        } else if (auto progress = event->getProgress()) {
            //this->updateProgressLabel(progress->downloadProgress().value_or(0.f));
        }
    }
    bool init(std::shared_ptr<MyCustomSettingV3> setting, float width) {
        if (!SettingValueNodeV3::init(setting, width))
            return false;

        auto linkButton = CCMenuItemExt::createSpriteExtra(ButtonSprite::create("Link"),[this](auto self){
            ConfirmAlertLayer::create([this](bool btn2){
                if (btn2){
                    m_linkListener.bind(this, &MyCustomSettingNodeV3::handleLinking);
                    m_linkListener.setFilter(AuthManager::get().linkAccount(m_linkTokenInput->getString()));
                }
            },"Warning!","This process is <cr>irreversible</c> and will link your accounts <cy>forever</c>!\nAre you sure you want to proceed?","No","Yes")->show();
            
        });
        m_linkTokenInput = TextInput::create(150,"Link secret...","bigFont.fnt");
        m_linkTokenInput->setFilter("QWERTYUIOPASDFGHJKLZXCVBNMqweryuiopasdfghjklzxcvbnm1234567890_-.");
        
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
    void onToggle(CCObject* sender) {
    }

public:
    static MyCustomSettingNodeV3* create(std::shared_ptr<MyCustomSettingV3> setting, float width) {
        auto ret = new MyCustomSettingNodeV3();
        if (ret->init(setting, width)) {
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