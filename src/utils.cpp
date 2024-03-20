#include "utils.h"
#include <Geode/Geode.hpp>

using namespace geode::prelude;

int levelthumbs::getQualityMultiplier(){
    auto scaleFactor = CCDirector::sharedDirector()->getContentScaleFactor();
	switch ((int)scaleFactor) {
	case 1:
		return 4;
		break;
	case 2:
		return 2;
		break;
	case 4:
		return 1;
		break;
	}
	return 1;
}