#include "WaveVisualizer.hpp"
#include "../config/VisualizerColor.hpp"
#include "../config/VisualizerOpacity.hpp"
#include "../config/VisualizerPower.hpp"

#include <Geode/Geode.hpp>

#include <algorithm>

using namespace geode::prelude;

namespace geovision {

namespace {
constexpr float kWaveWidthRatio = 0.72f;
constexpr float kMinimumAmplitude = 8.f;
constexpr float kMaximumAmplitude = 48.f;
constexpr float kWaveThickness = 3.f;
} // namespace

WaveVisualizer::~WaveVisualizer() {
	cleanup();
}

bool WaveVisualizer::init(CCNode* parent) {
	if (!parent) return false;
	if (m_drawNode) cleanup();

	auto* drawNode = CCDrawNode::create();
	if (!drawNode) return false;
	drawNode->setBlendFunc({ GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA });
	drawNode->setVisible(true);
	parent->addChild(drawNode, 10000);
	drawNode->retain();
	m_drawNode = drawNode;
	setVisualizerOpacityChangedCallback([this] {
		if (m_hasFrame) update(0.f, m_lastFrame);
	});
	setVisualizerPowerChangedCallback([this] {
		if (m_hasFrame) update(0.f, m_lastFrame);
	});
	setVisualizerColorChangedCallback([this] {
		if (m_hasFrame) update(0.f, m_lastFrame);
	});
	setReducedVisualizerResourcesChangedCallback([this] {
		if (m_hasFrame) update(0.f, m_lastFrame);
	});
	return true;
}

void WaveVisualizer::update(float dt, AudioFrame const& frame) {
	if (!m_drawNode) return;
	m_lastFrame = frame;
	m_hasFrame = true;
	const auto visualFrame = smoothVisualFrame(dt, frame);
	m_drawNode->clear();

	const auto winSize = CCDirector::sharedDirector()->getWinSize();
	const float width = winSize.width * kWaveWidthRatio;
	const float startX = (winSize.width - width) * 0.5f;
	const float centerY = winSize.height * 0.2f;
	const bool reducedDetail = reduceVisualizerResourcesEnabled();
	const std::size_t pointCount = reducedDetail ? (kAudioBandCount + 1) / 2 + 1 : kAudioBandCount;
	const float stepX = width / static_cast<float>(pointCount - 1);
	const bool transparencyEnabled = visualizerTransparencyEnabled();

	for (std::size_t i = 0; i + 1 < pointCount; ++i) {
		const auto [firstBand, endBand] = reducedDetail
			? visualizerReducedRange(i, pointCount, kAudioBandCount)
			: std::pair<std::size_t, std::size_t>{ i, i + 1 };
		const auto [nextFirstBand, nextEndBand] = reducedDetail
			? visualizerReducedRange(i + 1, pointCount, kAudioBandCount)
			: std::pair<std::size_t, std::size_t>{ i + 1, i + 2 };
		float firstBandEnergy = 0.f;
		float secondBandEnergy = 0.f;
		for (std::size_t band = firstBand; band < endBand; ++band)
			firstBandEnergy += visualFrame.valid ? std::clamp(visualFrame.bands[band], 0.f, 1.f) : 0.f;
		for (std::size_t band = nextFirstBand; band < nextEndBand; ++band)
			secondBandEnergy += visualFrame.valid ? std::clamp(visualFrame.bands[band], 0.f, 1.f) : 0.f;
		firstBandEnergy /= static_cast<float>(std::max<std::size_t>(1, endBand - firstBand));
		secondBandEnergy /= static_cast<float>(std::max<std::size_t>(1, nextEndBand - nextFirstBand));
		const float colorPosition = (static_cast<float>(firstBand + endBand - 1) * 0.5f +
			static_cast<float>(nextFirstBand + nextEndBand - 1) * 0.5f) /
			(2.f * static_cast<float>(kAudioBandCount - 1));
		const auto configuredColor = visualizerColorAt(colorPosition);
		const float red = configuredColor.r / 255.f;
		const float green = configuredColor.g / 255.f;
		const float blue = configuredColor.b / 255.f;
		const ccColor4F waveColor = { red, green, blue, applyVisualizerOpacity(transparencyEnabled ? 0.95f : 1.f) };
		const ccColor4F reflectionColor = { red, green, blue, applyVisualizerOpacity(transparencyEnabled ? 0.65f : 1.f) };
		const float firstEnergy = visualFrame.valid ? applyVisualizerPower(firstBandEnergy) : 0.f;
		const float secondEnergy = visualFrame.valid ? applyVisualizerPower(secondBandEnergy) : 0.f;
		const float firstAmplitude = kMinimumAmplitude + firstEnergy * (kMaximumAmplitude - kMinimumAmplitude);
		const float secondAmplitude = kMinimumAmplitude + secondEnergy * (kMaximumAmplitude - kMinimumAmplitude);
		const float firstX = startX + static_cast<float>(i) * stepX;
		const float secondX = firstX + stepX;

		m_drawNode->drawSegment(
			{firstX, centerY + firstAmplitude},
			{secondX, centerY + secondAmplitude},
			kWaveThickness,
			waveColor
		);
		m_drawNode->drawSegment(
			{firstX, centerY - firstAmplitude},
			{secondX, centerY - secondAmplitude},
			kWaveThickness,
			reflectionColor
		);
	}
}

void WaveVisualizer::cleanup() {
	setVisualizerOpacityChangedCallback({});
	setVisualizerPowerChangedCallback({});
	setVisualizerColorChangedCallback({});
	setReducedVisualizerResourcesChangedCallback({});
	if (!m_drawNode) return;
	m_drawNode->removeFromParentAndCleanup(true);
	m_drawNode->release();
	m_drawNode = nullptr;
}

cocos2d::CCRect WaveVisualizer::getBounds(cocos2d::CCSize winSize) const {
	// CCDrawNode::drawSegment's third argument is its radius.
	const float strokeRadius = kWaveThickness;
	const float width = winSize.width * kWaveWidthRatio;
	const float centerY = winSize.height * 0.2f;
	const float amplitude = kMaximumAmplitude + strokeRadius;
	return {
		(winSize.width - width) * 0.5f - strokeRadius,
		centerY - amplitude,
		width + strokeRadius * 2.f,
		amplitude * 2.f,
	};
}

} // namespace geovision
