#pragma once

#include <Geode/Geode.hpp>

#include <algorithm>
#include <functional>

namespace geovision {

inline std::function<void()> g_visualizerOpacityChanged;

inline void setVisualizerOpacityChangedCallback(std::function<void()> callback) {
	g_visualizerOpacityChanged = std::move(callback);
}

inline float& visualizerOpacity() {
	static float opacity = [] {
		return std::clamp(geode::Mod::get()->getSavedValue<float>("visualizer-opacity", 100.f), 0.f, 100.f);
	}();
	return opacity;
}

inline void setVisualizerOpacity(float opacity) {
	visualizerOpacity() = std::clamp(opacity, 0.f, 100.f);
	geode::Mod::get()->setSavedValue("visualizer-opacity", visualizerOpacity());
	if (g_visualizerOpacityChanged) g_visualizerOpacityChanged();
}

inline float applyVisualizerOpacity(float alpha) {
	return alpha * visualizerOpacity() / 100.f;
}

} // namespace geovision
