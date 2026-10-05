#pragma once

#include <Geode/Geode.hpp>

#include <algorithm>
#include <functional>

namespace geovision {

inline std::function<void()> g_visualizerPowerChanged;

inline void setVisualizerPowerChangedCallback(std::function<void()> callback) {
	g_visualizerPowerChanged = std::move(callback);
}

inline float& visualizerPower() {
	static float power = [] {
		return std::clamp(geode::Mod::get()->getSavedValue<float>("visualizer-power", 100.f), 0.f, 200.f);
	}();
	return power;
}

inline void setVisualizerPower(float power) {
	visualizerPower() = std::clamp(power, 0.f, 200.f);
	geode::Mod::get()->setSavedValue("visualizer-power", visualizerPower());
	if (g_visualizerPowerChanged) g_visualizerPowerChanged();
}

inline float applyVisualizerPower(float audioResponse) {
	return audioResponse * visualizerPower() / 100.f;
}

} // namespace geovision
