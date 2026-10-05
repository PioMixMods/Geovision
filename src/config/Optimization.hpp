#pragma once

#include <Geode/Geode.hpp>

#include <functional>

namespace geovision {

inline std::function<void()>& reducedVisualizerResourcesChangedCallback() {
	static std::function<void()> callback;
	return callback;
}

inline void setReducedVisualizerResourcesChangedCallback(std::function<void()> callback) {
	reducedVisualizerResourcesChangedCallback() = std::move(callback);
}

inline bool& avoidDuplicateRedrawEnabled() {
	static bool enabled = geode::Mod::get()->getSavedValue<bool>(
		"optimization-avoid-duplicate-redraw", true
	);
	return enabled;
}

inline void setAvoidDuplicateRedrawEnabled(bool enabled) {
	avoidDuplicateRedrawEnabled() = enabled;
	geode::Mod::get()->setSavedValue("optimization-avoid-duplicate-redraw", enabled);
}

inline bool& reduceVisualizerResourcesEnabled() {
	static bool enabled = geode::Mod::get()->getSavedValue<bool>(
		"optimization-reduce-visualizer-resources", false
	);
	return enabled;
}

inline void setReduceVisualizerResourcesEnabled(bool enabled) {
	if (reduceVisualizerResourcesEnabled() == enabled) return;
	reduceVisualizerResourcesEnabled() = enabled;
	geode::Mod::get()->setSavedValue("optimization-reduce-visualizer-resources", enabled);
	if (reducedVisualizerResourcesChangedCallback()) {
		reducedVisualizerResourcesChangedCallback()();
	}
}

} // namespace geovision
