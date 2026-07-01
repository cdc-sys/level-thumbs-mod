#pragma once
#include <argon/argon.hpp>
#include <Geode/Geode.hpp>
#include <Geode/utils/web.hpp>
#include "SettingsManager.hpp"
#include "../layers/LoadingOverlay.hpp"


enum class ThumbnailRole {
    NONE = 0,
    USER = 1,
    VERIFIED = 2,
    MODERATOR = 3,
    ADMIN = 4,
    OWNER = 5
};
struct ThumbnailRoleInfo {
    std::string badge_sprite;
    std::string name;
    std::string description;
};

static std::unordered_map<ThumbnailRole,ThumbnailRoleInfo> THUMBNAIL_ROLES = {
    {ThumbnailRole::USER,{"LT_Badge_THE.png"_spr,"User","This user has submitted thumbnails to the <co>Level Thumbnails</c> mod."}},
    {ThumbnailRole::VERIFIED,{"LT_Badge_VerThumbnailer.png"_spr,"Verified Thumbnailer","This user has submitted a lot of quality thumbnails to the <co>Level Thumbnails</c> mod."}},
    {ThumbnailRole::MODERATOR,{"LT_Badge_ThumbnailMod.png"_spr,"Thumbnail Moderator","This user is authorized to review thumbnails for the <co>Level Thumbnails</c> mod."}},
    {ThumbnailRole::ADMIN,{"LT_Badge_ThumbnailAdmin.png"_spr,"Thumbnail Admin","This user is authorized to <cy>lock levels</c> and promote new <cj>Moderators</c> for the <co>Level Thumbnails</c> mod."}},
    {ThumbnailRole::OWNER,{"LT_Badge_Owner.png"_spr,"Owner","This user is an owner of the <co>Level Thumbnails</c> mod."}}
};

static std::unordered_map<std::string, ThumbnailRole> THUMBNAIL_ROLE_SERVER_NAMES = {
    {"user",ThumbnailRole::USER},
    {"verified",ThumbnailRole::VERIFIED},
    {"moderator",ThumbnailRole::MODERATOR},
    {"admin",ThumbnailRole::ADMIN},
    {"owner",ThumbnailRole::OWNER}
};

inline ThumbnailRole getRoleByName(std::string role) {
    return THUMBNAIL_ROLE_SERVER_NAMES[role];
}

inline ThumbnailRoleInfo getRoleInfoByName(std::string role) {
    return THUMBNAIL_ROLES[getRoleByName(role)];
}

class AuthManager {
private:
    AuthManager() = default;
public:

    ThumbnailRole myRole = ThumbnailRole::NONE;
    bool checkedRole;

    AuthManager(AuthManager const&) = delete;
    AuthManager(AuthManager&&) = delete;
    AuthManager& operator=(AuthManager const&) = delete;
    AuthManager& operator=(AuthManager&&) = delete;

    using LoginResult = geode::Result<std::string>;
    using LinkResult = geode::Result<std::string>;
    using UploadResult = geode::Result<std::string>;

    using LoginFuture = arc::Future<LoginResult>;
    using LinkFuture = arc::Future<LinkResult>;
    using UploadFuture = arc::Future<UploadResult>;

    static bool isLoggedIn();
    LoginFuture login();
    UploadFuture uploadThumbnail(std::string_view filename, int levelID, std::string note, geode::Function<void(geode::ZStringView)> onProgress = nullptr);
    LinkFuture linkAccount(std::string linkSecret);
    
    static std::string getToken();

    static AuthManager& get();
};