#pragma once

#include <Geode/Geode.hpp>

namespace geovision {

inline bool matchBackgroundColorEnabled() {
	return geode::Mod::get()->getSavedValue<bool>("match-background-color", false);
}

inline void setMatchBackgroundColorEnabled(bool enabled) {
	geode::Mod::get()->setSavedValue("match-background-color", enabled);
}

} // namespace geovision
