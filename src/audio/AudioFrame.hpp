#pragma once

#include <array>
#include <cstddef>

namespace geovision {

inline constexpr std::size_t kAudioBandCount = 16;

struct AudioFrame {
	std::array<float, kAudioBandCount> bands{};
	float bass = 0.f;
	float volume = 0.f;
	bool valid = false;
};

} // namespace geovision
