#pragma once

#include "../audio/AudioFrame.hpp"
#include "../config/VisualizerSmoothing.hpp"
#include "../config/Optimization.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <utility>

namespace cocos2d {
class CCNode;
class CCSize;
class CCRect;
}

namespace geovision {

enum class VisualizerType {
	Bars,
	Wave,
	Radial,
};

inline VisualizerType g_selectedVisualizerType = VisualizerType::Bars;

// Evenly partition full-resolution visual data into fewer representative groups.
// Shared by visualizers so future implementations can use the same reduction policy.
inline std::pair<std::size_t, std::size_t> visualizerReducedRange(
	std::size_t outputIndex,
	std::size_t outputCount,
	std::size_t inputCount
) {
	if (outputCount == 0 || inputCount == 0) return { 0, 0 };
	return { outputIndex * inputCount / outputCount,
		(outputIndex + 1) * inputCount / outputCount };
}

class Visualizer {
public:
	virtual ~Visualizer() = default;
	virtual bool init(cocos2d::CCNode* parent) = 0;
	virtual void update(float dt, AudioFrame const& frame) = 0;
	virtual void cleanup() = 0;
	virtual cocos2d::CCRect getBounds(cocos2d::CCSize winSize) const = 0;

protected:
	AudioFrame smoothVisualFrame(float dt, AudioFrame const& target) {
		const float amount = visualizerSmoothing() / 100.f;
		if (!m_hasVisualFrame || amount <= 0.f || !target.valid) {
			m_visualFrame = target;
			m_hasVisualFrame = true;
			return m_visualFrame;
		}

		const float timeConstant = 0.8f * amount * amount;
		const float alpha = 1.f - std::exp(-std::max(dt, 0.f) / timeConstant);
		for (std::size_t i = 0; i < kAudioBandCount; ++i) {
			m_visualFrame.bands[i] += (target.bands[i] - m_visualFrame.bands[i]) * alpha;
		}
		m_visualFrame.bass += (target.bass - m_visualFrame.bass) * alpha;
		m_visualFrame.volume += (target.volume - m_visualFrame.volume) * alpha;
		m_visualFrame.valid = target.valid;
		return m_visualFrame;
	}

private:
	AudioFrame m_visualFrame;
	bool m_hasVisualFrame = false;
};

} // namespace geovision
