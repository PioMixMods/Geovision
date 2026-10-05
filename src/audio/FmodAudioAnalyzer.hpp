#pragma once

#include "AudioAnalyzer.hpp"

#include <array>

namespace FMOD {
class Channel;
class DSP;
class Sound;
} // namespace FMOD

namespace geovision {

class FmodAudioAnalyzer final : public AudioAnalyzer {
public:
	~FmodAudioAnalyzer() override;

	AudioFrame update(float dt) override;
	void reset() override;

private:
	struct BandBinRange {
		int first = 0;
		int last = 0;
	};

	bool attachTo(FMOD::Channel* channel, int musicID, FMOD::Sound* sound);
	void rebuildBandRanges(int binCount);
	void detach();

	FMOD::Channel* m_channel = nullptr;
	FMOD::DSP* m_fft = nullptr;
	FMOD::Sound* m_sound = nullptr;
	int m_musicID = -1;
	bool m_waitingForChannel = false;
	int m_binCount = 0;
	std::array<BandBinRange, kAudioBandCount> m_bandRanges{};
	AudioFrame m_smoothedFrame;
};

} // namespace geovision
