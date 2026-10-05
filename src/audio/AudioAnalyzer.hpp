#pragma once

#include "AudioFrame.hpp"

namespace geovision {

class AudioAnalyzer {
public:
	virtual ~AudioAnalyzer() = default;
	virtual AudioFrame update(float dt) = 0;
	virtual void reset() = 0;
};

} // namespace geovision
