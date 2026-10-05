#pragma once

#include "MatchBackgroundColor.hpp"
#include "Optimization.hpp"
#include "VisualizerColor.hpp"
#include "VisualizerOpacity.hpp"
#include "VisualizerPosition.hpp"
#include "VisualizerPower.hpp"
#include "VisualizerSensitivity.hpp"
#include "VisualizerSmoothing.hpp"
#include "../visualizers/Visualizer.hpp"

#include <Geode/Geode.hpp>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <string>
#include <vector>

namespace geovision {

struct VisualizerPreset {
	std::string name;
	VisualizerType visualizer = VisualizerType::Bars;
	float positionX = 0.f;
	float positionY = 0.f;
	float size = 1.f;
	VisualizerOrientation orientation = VisualizerOrientation::Vertical;
	cocos2d::ccColor3B color1 = { 41, 217, 255 };
	cocos2d::ccColor3B color2 = { 41, 217, 255 };
	bool useColor2 = false;
	float opacity = 100.f;
	float power = 100.f;
	float sensitivity = 100.f;
	float smoothing = 0.f;
	bool matchBackgroundColor = false;
	bool reduceVisualizerResources = false;
	bool avoidDuplicateRedraw = true;
	bool transparencyEnabled = true;
};

inline constexpr char kVisualizerPresetsKey[] = "geovision-presets-v1";

inline std::vector<std::string> visualizerPresetRecords() {
	return geode::Mod::get()->getSavedValue<std::vector<std::string>>(
		kVisualizerPresetsKey, std::vector<std::string>{}
	);
}

inline void setVisualizerPresetRecords(std::vector<std::string> const& records) {
	geode::Mod::get()->setSavedValue(kVisualizerPresetsKey, records);
}

inline std::vector<std::string> splitVisualizerPresetRecord(std::string const& record) {
	std::vector<std::string> fields;
	std::size_t begin = 0;
	while (begin <= record.size()) {
		const auto end = record.find('|', begin);
		fields.emplace_back(record.substr(begin, end == std::string::npos ? end : end - begin));
		if (end == std::string::npos) break;
		begin = end + 1;
	}
	return fields;
}

inline std::vector<std::string> visualizerPresetNames() {
	std::vector<std::string> names;
	for (auto const& record : visualizerPresetRecords()) {
		auto fields = splitVisualizerPresetRecord(record);
		if (fields.empty() || fields[0].empty()) continue;
		if (std::find(names.begin(), names.end(), fields[0]) == names.end()) {
			names.push_back(fields[0]);
		}
	}
	return names;
}

inline bool hasVisualizerPreset(std::string const& name) {
	const auto names = visualizerPresetNames();
	return std::find(names.begin(), names.end(), name) != names.end();
}

inline std::string sanitizeVisualizerPresetName(std::string name) {
	for (auto& character : name) {
		if (character == '|' || character == '\n' || character == '\r') character = ' ';
	}
	const auto first = name.find_first_not_of(" \t");
	if (first == std::string::npos) return {};
	const auto last = name.find_last_not_of(" \t");
	name = name.substr(first, last - first + 1);
	if (name.size() > 32) name.resize(32);
	return name;
}

inline VisualizerPreset captureVisualizerPreset(std::string name) {
	VisualizerPreset preset;
	preset.name = sanitizeVisualizerPresetName(std::move(name));
	preset.visualizer = g_selectedVisualizerType;
	const auto& position = visualizerPositionState();
	preset.positionX = position.x;
	preset.positionY = position.y;
	preset.size = position.size;
	preset.orientation = position.orientation;
	preset.color1 = visualizerColor();
	preset.color2 = visualizerColor2();
	preset.useColor2 = visualizerColor2Enabled();
	preset.opacity = visualizerOpacity();
	preset.power = visualizerPower();
	preset.sensitivity = visualizerSensitivity();
	preset.smoothing = visualizerSmoothing();
	preset.matchBackgroundColor = matchBackgroundColorEnabled();
	preset.reduceVisualizerResources = reduceVisualizerResourcesEnabled();
	preset.avoidDuplicateRedraw = avoidDuplicateRedrawEnabled();
	preset.transparencyEnabled = visualizerTransparencyEnabled();
	return preset;
}

inline std::string serializeVisualizerPreset(VisualizerPreset const& preset) {
	return preset.name + "|" +
		std::to_string(static_cast<int>(preset.visualizer)) + "|" +
		std::to_string(preset.positionX) + "|" + std::to_string(preset.positionY) + "|" +
		std::to_string(preset.size) + "|" +
		std::to_string(static_cast<int>(preset.orientation)) + "|" +
		std::to_string(static_cast<int>(preset.color1.r)) + "|" +
		std::to_string(static_cast<int>(preset.color1.g)) + "|" +
		std::to_string(static_cast<int>(preset.color1.b)) + "|" +
		std::to_string(static_cast<int>(preset.color2.r)) + "|" +
		std::to_string(static_cast<int>(preset.color2.g)) + "|" +
		std::to_string(static_cast<int>(preset.color2.b)) + "|" +
		std::to_string(preset.useColor2 ? 1 : 0) + "|" +
		std::to_string(preset.opacity) + "|" + std::to_string(preset.power) + "|" +
		std::to_string(preset.sensitivity) + "|" + std::to_string(preset.smoothing) + "|" +
		std::to_string(preset.matchBackgroundColor ? 1 : 0) + "|" +
		std::to_string(preset.reduceVisualizerResources ? 1 : 0) + "|" +
		std::to_string(preset.avoidDuplicateRedraw ? 1 : 0) + "|" +
		std::to_string(preset.transparencyEnabled ? 1 : 0);
}

inline float parsePresetFloat(std::vector<std::string> const& fields, std::size_t index, float fallback,
	float minimum, float maximum) {
	if (index >= fields.size()) return fallback;
	char* end = nullptr;
	const float value = std::strtof(fields[index].c_str(), &end);
	if (end == fields[index].c_str() || *end != '\0' || !std::isfinite(value)) return fallback;
	return std::clamp(value, minimum, maximum);
}

inline int parsePresetInt(std::vector<std::string> const& fields, std::size_t index,
	int fallback, int minimum, int maximum) {
	if (index >= fields.size()) return fallback;
	char* end = nullptr;
	const long value = std::strtol(fields[index].c_str(), &end, 10);
	if (end == fields[index].c_str() || *end != '\0') return fallback;
	return static_cast<int>(std::clamp(value, static_cast<long>(minimum), static_cast<long>(maximum)));
}

inline bool parsePresetBool(std::vector<std::string> const& fields, std::size_t index, bool fallback) {
	if (index >= fields.size()) return fallback;
	if (fields[index] == "1") return true;
	if (fields[index] == "0") return false;
	return fallback;
}

inline VisualizerPreset parseVisualizerPreset(std::string const& record) {
	const auto fields = splitVisualizerPresetRecord(record);
	VisualizerPreset preset;
	if (!fields.empty()) preset.name = sanitizeVisualizerPresetName(fields[0]);
	preset.visualizer = static_cast<VisualizerType>(parsePresetInt(fields, 1, 0, 0, 2));
	preset.positionX = parsePresetFloat(fields, 2, 0.f, -1.f, 1.f);
	preset.positionY = parsePresetFloat(fields, 3, 0.f, -1.f, 1.f);
	preset.size = parsePresetFloat(fields, 4, 1.f, 0.5f, 1.5f);
	preset.orientation = static_cast<VisualizerOrientation>(parsePresetInt(fields, 5, 0, 0, 1));
	preset.color1 = {
		static_cast<GLubyte>(parsePresetInt(fields, 6, 41, 0, 255)),
		static_cast<GLubyte>(parsePresetInt(fields, 7, 217, 0, 255)),
		static_cast<GLubyte>(parsePresetInt(fields, 8, 255, 0, 255)),
	};
	preset.color2 = {
		static_cast<GLubyte>(parsePresetInt(fields, 9, 41, 0, 255)),
		static_cast<GLubyte>(parsePresetInt(fields, 10, 217, 0, 255)),
		static_cast<GLubyte>(parsePresetInt(fields, 11, 255, 0, 255)),
	};
	preset.useColor2 = parsePresetBool(fields, 12, false);
	preset.opacity = parsePresetFloat(fields, 13, 100.f, 0.f, 100.f);
	preset.power = parsePresetFloat(fields, 14, 100.f, 0.f, 200.f);
	preset.sensitivity = parsePresetFloat(fields, 15, 100.f, 0.f, 200.f);
	preset.smoothing = parsePresetFloat(fields, 16, 0.f, 0.f, 100.f);
	preset.matchBackgroundColor = parsePresetBool(fields, 17, false);
	preset.reduceVisualizerResources = parsePresetBool(fields, 18, false);
	preset.avoidDuplicateRedraw = parsePresetBool(fields, 19, true);
	preset.transparencyEnabled = parsePresetBool(fields, 20, true);
	return preset;
}

inline void saveVisualizerPreset(std::string const& name) {
	auto preset = captureVisualizerPreset(name);
	if (preset.name.empty()) return;
	auto records = visualizerPresetRecords();
	const auto serialized = serializeVisualizerPreset(preset);
	auto found = std::find_if(records.begin(), records.end(), [&](std::string const& record) {
		const auto fields = splitVisualizerPresetRecord(record);
		return !fields.empty() && fields[0] == preset.name;
	});
	if (found == records.end()) records.push_back(serialized);
	else *found = serialized;
	setVisualizerPresetRecords(records);
}

inline bool loadVisualizerPreset(std::string const& name) {
	const auto records = visualizerPresetRecords();
	auto found = std::find_if(records.begin(), records.end(), [&](std::string const& record) {
		const auto fields = splitVisualizerPresetRecord(record);
		return !fields.empty() && fields[0] == name;
	});
	if (found == records.end()) return false;
	const auto preset = parseVisualizerPreset(*found);

	setVisualizerPositionPreview(false);
	g_selectedVisualizerType = preset.visualizer;
	updateVisualizerPositionX(preset.positionX);
	updateVisualizerPositionY(preset.positionY);
	updateVisualizerPositionSize(preset.size);
	setVisualizerOrientation(preset.orientation);
	saveVisualizerPosition();
	setVisualizerColor(preset.color1);
	setVisualizerColor2(preset.color2);
	setVisualizerColor2Enabled(preset.useColor2);
	setVisualizerTransparencyEnabled(preset.transparencyEnabled);
	setVisualizerOpacity(preset.opacity);
	setVisualizerPower(preset.power);
	setVisualizerSensitivity(preset.sensitivity);
	setVisualizerSmoothing(preset.smoothing);
	setMatchBackgroundColorEnabled(preset.matchBackgroundColor);
	setReduceVisualizerResourcesEnabled(preset.reduceVisualizerResources);
	setAvoidDuplicateRedrawEnabled(preset.avoidDuplicateRedraw);
	return true;
}

inline bool deleteVisualizerPreset(std::string const& name) {
	auto records = visualizerPresetRecords();
	const auto oldSize = records.size();
	std::erase_if(records, [&](std::string const& record) {
		const auto fields = splitVisualizerPresetRecord(record);
		return !fields.empty() && fields[0] == name;
	});
	if (records.size() == oldSize) return false;
	setVisualizerPresetRecords(records);
	return true;
}

} // namespace geovision
