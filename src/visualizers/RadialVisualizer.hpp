#pragma once

#include "Visualizer.hpp"

namespace cocos2d {
class CCDrawNode;
}

namespace geovision {

class RadialVisualizer final : public Visualizer {
public:
	~RadialVisualizer() override;

	bool init(cocos2d::CCNode* parent) override;
	void update(float dt, AudioFrame const& frame) override;
	void cleanup() override;
	cocos2d::CCRect getBounds(cocos2d::CCSize winSize) const override;

private:
	cocos2d::CCDrawNode* m_drawNode = nullptr;
	AudioFrame m_lastFrame;
	bool m_hasFrame = false;
};

} // namespace geovision
