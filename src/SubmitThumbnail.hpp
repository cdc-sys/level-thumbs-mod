#include <Geode/Geode.hpp>
#include <Geode/utils/web.hpp>
#include "LoadingOverlay.hpp"

using namespace geode::prelude;

class SubmitThumbnail{
private:
    int m_id;
    EventListener<web::WebTask> m_eventListener;
    std::string m_argonToken;
    LoadingOverlay* m_loadingOverlay;

    void step1();
    void step2(std::string token);
public:
    SubmitThumbnail(int id,std::string argonToken,LoadingOverlay* loadingOverlay);
};