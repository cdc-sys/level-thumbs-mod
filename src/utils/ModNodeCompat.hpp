#pragma once
#include <cocos2d.h>

// megahack
namespace core { class Poller : public cocos2d::CCNode {}; }
namespace status { class Manager : public cocos2d::CCNodeRGBA {}; }
class ShowTrajectory : public cocos2d::CCNode {};
class NoclipTint : public cocos2d::CCLayerColor {};

// globed
namespace globed { class ProgressArrow : public cocos2d::CCNode {}; };

// qolmod
class HitboxNode : public cocos2d::CCNode {};
namespace qolmod { class TrajectoryNode : public cocos2d::CCNode {}; };