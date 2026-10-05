#include "FmodAudioAnalyzer.hpp"
#include "../config/VisualizerSensitivity.hpp"

#include <Geode/Geode.hpp>
#include <Geode/binding/FMODAudioEngine.hpp>
#include <Geode/fmod/fmod_dsp.h>
#include <Geode/fmod/fmod_dsp_effects.h>

#include <algorithm>
#include <cmath>

using namespace geode::prelude;

namespace geovision {

namespace {
constexpr int kLevelMusicID = 0;
constexpr int kFftWindowSize = 512;
constexpr float kMagnitudeScale = 24.f;
constexpr float kAttackSeconds = 0.035f;
constexpr float kReleaseSeconds = 0.18f;

float smoothToward(float current, float target, float dt) {
	const float seconds = target > current ? kAttackSeconds : kReleaseSeconds;
	const float alpha = 1.f - std::exp(-std::max(dt, 0.f) / seconds);
	return current + (target - current) * alpha;
}
} // namespace

FmodAudioAnalyzer::~FmodAudioAnalyzer() {
	reset();
}

void FmodAudioAnalyzer::reset() {
	detach();
	m_smoothedFrame = {};
	m_waitingForChannel = false;
}

void FmodAudioAnalyzer::detach() {
	if (m_channel && m_fft) {
		// A song trigger may already have removed the DSP or stopped this channel.
		int dspIndex = -1;
		if (m_channel->getDSPIndex(m_fft, &dspIndex) == FMOD_OK) {
			m_channel->removeDSP(m_fft);
		}
	}
	if (m_fft) {
		m_fft->release();
	}
	m_channel = nullptr;
	m_fft = nullptr;
	m_sound = nullptr;
	m_musicID = -1;
	m_binCount = 0;
}

void FmodAudioAnalyzer::rebuildBandRanges(int binCount) {
	m_binCount = binCount;
	if (binCount <= 1) {
		m_bandRanges = {};
		return;
	}

	const float logBinCount = std::log(static_cast<float>(binCount));
	for (std::size_t band = 0; band < kAudioBandCount; ++band) {
		int firstBin = std::max(1, static_cast<int>(std::floor(std::exp(
			logBinCount * static_cast<float>(band) / static_cast<float>(kAudioBandCount)
		))));
		int lastBin = std::max(firstBin + 1, static_cast<int>(std::floor(std::exp(
			logBinCount * static_cast<float>(band + 1) / static_cast<float>(kAudioBandCount)
		))));
		lastBin = std::min(lastBin, binCount);
		m_bandRanges[band] = { firstBin, lastBin };
	}
}

bool FmodAudioAnalyzer::attachTo(FMOD::Channel* channel, int musicID, FMOD::Sound* sound) {
	if (!channel) return false;

	FMOD::System* system = nullptr;
	if (channel->getSystemObject(&system) != FMOD_OK || !system) return false;

	FMOD::DSP* fft = nullptr;
	if (system->createDSPByType(FMOD_DSP_TYPE_FFT, &fft) != FMOD_OK || !fft) return false;

	if (fft->setParameterInt(FMOD_DSP_FFT_WINDOWSIZE, kFftWindowSize) != FMOD_OK) {
		fft->release();
		return false;
	}

	int dspCount = 0;
	if (channel->getNumDSPs(&dspCount) != FMOD_OK ||
		channel->addDSP(dspCount, fft) != FMOD_OK) {
		fft->release();
		return false;
	}

	m_channel = channel;
	m_fft = fft;
	m_sound = sound;
	m_musicID = musicID;
	rebuildBandRanges(kFftWindowSize / 2);
	log::info("Geovision: FFT DSP attached to active music ID {}", musicID);
	return true;
}

AudioFrame FmodAudioAnalyzer::update(float dt) {
	auto* engine = FMODAudioEngine::get();
	if (!engine) {
		reset();
		return {};
	}

	// Song Trigger channels are not necessarily ID 0. Prefer the currently
	// audible FMOD music channel, while retaining ID 0 as the fallback.
	FMOD::Channel* activeChannel = nullptr;
	int activeMusicID = -1;
	float bestAudibility = -1.f;
	FMOD::Channel* fallbackChannel = nullptr;
	int fallbackMusicID = -1;
	auto considerMusicID = [&](int musicID) {
		auto* channel = engine->getActiveMusicChannel(musicID);
		if (!channel) return;
		bool playing = false;
		if (channel->isPlaying(&playing) != FMOD_OK || !playing) return;
		if (!fallbackChannel) {
			fallbackChannel = channel;
			fallbackMusicID = musicID;
		}
		if (musicID == kLevelMusicID) {
			fallbackChannel = channel;
			fallbackMusicID = musicID;
		}

		float audibility = 0.f;
		if (channel->getAudibility(&audibility) != FMOD_OK) return;
		const bool isCurrentChannel = musicID == m_musicID && channel == m_channel;
		if (audibility > bestAudibility + 0.01f ||
			(isCurrentChannel && audibility + 0.02f >= bestAudibility)) {
			activeChannel = channel;
			activeMusicID = musicID;
			bestAudibility = audibility;
		}
	};
	considerMusicID(kLevelMusicID);
	for (auto const& music : engine->m_fmodMusic) {
		if (music.first != kLevelMusicID) considerMusicID(music.first);
	}
	if (!activeChannel) {
		activeChannel = fallbackChannel;
		activeMusicID = fallbackMusicID;
	}
	if (!activeChannel) {
		const bool wasWaiting = m_waitingForChannel;
		reset();
		m_waitingForChannel = true;
		if (!wasWaiting) log::debug("Geovision: waiting for an active level music channel");
		return {};
	}
	if (m_waitingForChannel) log::info("Geovision: active music channel is available again");
	m_waitingForChannel = false;

	FMOD::Sound* activeSound = nullptr;
	const bool hasActiveSound = activeChannel->getCurrentSound(&activeSound) == FMOD_OK && activeSound;
	int dspIndex = -1;
	const bool dspStillConnected = m_channel == activeChannel && m_fft &&
		activeChannel->getDSPIndex(m_fft, &dspIndex) == FMOD_OK;
	const bool channelChanged = activeChannel != m_channel;
	const bool musicIDChanged = activeMusicID != m_musicID;
	const bool soundChanged = hasActiveSound && m_sound && activeSound != m_sound;
	if (channelChanged || musicIDChanged || soundChanged || !dspStillConnected) {
		if (m_fft) {
			log::info(
				"Geovision: reconnecting FFT (channelChanged={}, musicIDChanged={}, soundChanged={}, dspConnected={})",
				channelChanged, musicIDChanged, soundChanged, dspStillConnected
			);
		}
		detach();
		if (!attachTo(activeChannel, activeMusicID, hasActiveSound ? activeSound : nullptr)) {
			m_smoothedFrame = {};
			return {};
		}
	} else if (hasActiveSound && !m_sound) {
		m_sound = activeSound;
	}
	if (!m_fft) return {};

	void* parameterData = nullptr;
	unsigned int dataLength = 0;
	if (m_fft->getParameterData(
			FMOD_DSP_FFT_SPECTRUMDATA,
			&parameterData,
			&dataLength,
		nullptr,
		0
		) != FMOD_OK ||
		!parameterData || dataLength < sizeof(FMOD_DSP_PARAMETER_FFT)) {
		return {};
	}

	auto const* spectrum = static_cast<FMOD_DSP_PARAMETER_FFT const*>(parameterData);
	if (spectrum->length <= 1 || spectrum->numchannels <= 0) return {};

	const int binCount = spectrum->length / 2;
	const int channelCount = std::min(spectrum->numchannels, 32);
	if (binCount <= 1) return {};
	if (binCount != m_binCount) rebuildBandRanges(binCount);

	AudioFrame frame;
	frame.valid = true;

	for (std::size_t band = 0; band < kAudioBandCount; ++band) {
		const auto& range = m_bandRanges[band];

		float sumSquares = 0.f;
		int sampleCount = 0;
		for (int channel = 0; channel < channelCount; ++channel) {
			if (!spectrum->spectrum[channel]) continue;
			for (int bin = range.first; bin < range.last; ++bin) {
				const float value = spectrum->spectrum[channel][bin];
				sumSquares += value * value;
				++sampleCount;
			}
		}

		const float magnitude = sampleCount > 0
			? std::sqrt(sumSquares / static_cast<float>(sampleCount))
			: 0.f;
		const float normalized = std::clamp(magnitude * kMagnitudeScale, 0.f, 1.f);
		m_smoothedFrame.bands[band] = smoothToward(m_smoothedFrame.bands[band], normalized, dt);
		frame.bands[band] = applyVisualizerSensitivity(m_smoothedFrame.bands[band]);
	}

	float bassEnergy = 0.f;
	float volume = 0.f;
	for (std::size_t band = 0; band < kAudioBandCount; ++band) {
		volume += frame.bands[band];
		if (band < 4) bassEnergy += frame.bands[band];
	}
	frame.bass = bassEnergy / 4.f;
	frame.volume = volume / static_cast<float>(kAudioBandCount);
	return frame;
}

} // namespace geovision
