#pragma once

#include <Geode/Geode.hpp>

#include <algorithm>
#include <cmath>

namespace geovision {

inline float& visualizerSensitivity() {
	static float sensitivity = [] {
		return std::clamp(geode::Mod::get()->getSavedValue<float>("visualizer-sensitivity", 100.f), 0.f, 200.f);
	}();
	return sensitivity;
}

inline void setVisualizerSensitivity(float sensitivity) {
	visualizerSensitivity() = std::clamp(sensitivity, 0.f, 200.f);
	geode::Mod::get()->setSavedValue("visualizer-sensitivity", visualizerSensitivity());
}

inline float applyVisualizerSensitivity(float audioLevel) {
	const float sensitivity = visualizerSensitivity();
	if (sensitivity <= 0.f) return 0.f;
	return std::pow(std::clamp(audioLevel, 0.f, 1.f), 100.f / sensitivity);
}

} // namespace geovision
