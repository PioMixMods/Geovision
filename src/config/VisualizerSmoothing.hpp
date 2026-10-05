#pragma once

#include <Geode/Geode.hpp>

#include <algorithm>

namespace geovision {

inline float& visualizerSmoothing() {
	static float smoothing = [] {
		return std::clamp(geode::Mod::get()->getSavedValue<float>("visualizer-smoothing", 0.f), 0.f, 100.f);
	}();
	return smoothing;
}

inline void setVisualizerSmoothing(float smoothing) {
	visualizerSmoothing() = std::clamp(smoothing, 0.f, 100.f);
	geode::Mod::get()->setSavedValue("visualizer-smoothing", visualizerSmoothing());
}

} // namespace geovision
