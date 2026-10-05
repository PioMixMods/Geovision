#pragma once

#include <Geode/Geode.hpp>

#include <algorithm>
#include <cmath>
#include <functional>

namespace geovision {

inline std::function<void()> g_visualizerColorChanged;
inline bool g_backgroundColorOverrideEnabled = false;
inline cocos2d::ccColor3B g_backgroundColorOverride = { 255, 255, 255 };

inline void setVisualizerColorChangedCallback(std::function<void()> callback) {
	g_visualizerColorChanged = std::move(callback);
}

inline void notifyVisualizerColorChanged() {
	if (g_visualizerColorChanged) g_visualizerColorChanged();
}

inline void setVisualizerBackgroundColorOverride(
	bool enabled,
	cocos2d::ccColor3B color = { 255, 255, 255 },
	bool notifyVisualizer = true
) {
	const bool changed = g_backgroundColorOverrideEnabled != enabled ||
		g_backgroundColorOverride.r != color.r ||
		g_backgroundColorOverride.g != color.g ||
		g_backgroundColorOverride.b != color.b;
	g_backgroundColorOverrideEnabled = enabled;
	g_backgroundColorOverride = color;
	if (changed && notifyVisualizer) notifyVisualizerColorChanged();
}

inline cocos2d::ccColor3B& visualizerColor() {
	static cocos2d::ccColor3B color = [] {
		auto* mod = geode::Mod::get();
		return cocos2d::ccColor3B{
			static_cast<GLubyte>(std::clamp(mod->getSavedValue<int>("visualizer-color-r", 41), 0, 255)),
			static_cast<GLubyte>(std::clamp(mod->getSavedValue<int>("visualizer-color-g", 217), 0, 255)),
			static_cast<GLubyte>(std::clamp(mod->getSavedValue<int>("visualizer-color-b", 255), 0, 255)),
		};
	}();
	return color;
}

inline void setVisualizerColor(cocos2d::ccColor3B color) {
	visualizerColor() = color;
	auto* mod = geode::Mod::get();
	mod->setSavedValue("visualizer-color-r", static_cast<int>(color.r));
	mod->setSavedValue("visualizer-color-g", static_cast<int>(color.g));
	mod->setSavedValue("visualizer-color-b", static_cast<int>(color.b));
	notifyVisualizerColorChanged();
}

inline cocos2d::ccColor3B& visualizerColor2() {
	static cocos2d::ccColor3B color = [] {
		auto* mod = geode::Mod::get();
		const auto firstColor = visualizerColor();
		return cocos2d::ccColor3B{
			static_cast<GLubyte>(std::clamp(mod->getSavedValue<int>("visualizer-color2-r", firstColor.r), 0, 255)),
			static_cast<GLubyte>(std::clamp(mod->getSavedValue<int>("visualizer-color2-g", firstColor.g), 0, 255)),
			static_cast<GLubyte>(std::clamp(mod->getSavedValue<int>("visualizer-color2-b", firstColor.b), 0, 255)),
		};
	}();
	return color;
}

inline void setVisualizerColor2(cocos2d::ccColor3B color) {
	visualizerColor2() = color;
	auto* mod = geode::Mod::get();
	mod->setSavedValue("visualizer-color2-r", static_cast<int>(color.r));
	mod->setSavedValue("visualizer-color2-g", static_cast<int>(color.g));
	mod->setSavedValue("visualizer-color2-b", static_cast<int>(color.b));
	notifyVisualizerColorChanged();
}

inline bool& visualizerColor2Enabled() {
	static bool enabled = geode::Mod::get()->getSavedValue<bool>("visualizer-color2-enabled", false);
	return enabled;
}

inline void setVisualizerColor2Enabled(bool enabled) {
	visualizerColor2Enabled() = enabled;
	geode::Mod::get()->setSavedValue("visualizer-color2-enabled", enabled);
	notifyVisualizerColorChanged();
}

inline cocos2d::ccColor3B visualizerColorAt(float position) {
	const auto first = g_backgroundColorOverrideEnabled
		? g_backgroundColorOverride
		: visualizerColor();
	if (!visualizerColor2Enabled()) return first;
	const auto second = visualizerColor2();
	const float t = std::clamp(position, 0.f, 1.f);
	return {
		static_cast<GLubyte>(std::lround(first.r + (second.r - first.r) * t)),
		static_cast<GLubyte>(std::lround(first.g + (second.g - first.g) * t)),
		static_cast<GLubyte>(std::lround(first.b + (second.b - first.b) * t)),
	};
}

inline bool& visualizerTransparencyEnabled() {
	static bool enabled = geode::Mod::get()->getSavedValue<bool>("visualizer-transparency-enabled", true);
	return enabled;
}

inline void setVisualizerTransparencyEnabled(bool enabled) {
	visualizerTransparencyEnabled() = enabled;
	geode::Mod::get()->setSavedValue("visualizer-transparency-enabled", enabled);
}

} // namespace geovision
