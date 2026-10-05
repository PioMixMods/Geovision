#include "RadialVisualizer.hpp"

#include "../config/VisualizerColor.hpp"
#include "../config/VisualizerOpacity.hpp"
#include "../config/VisualizerPower.hpp"

#include <Geode/Geode.hpp>

#include <algorithm>
#include <cmath>

using namespace geode::prelude;

namespace geovision {

namespace {
constexpr float kPi = 3.14159265358979323846f;
constexpr float kRadiusRatio = 0.12f;
constexpr float kAmplitudeRatio = 0.14f;
constexpr float kMinimumAmplitude = 8.f;
constexpr float kMaximumAmplitude = 58.f;
constexpr float kSegmentRadius = 3.5f;
} // namespace

RadialVisualizer::~RadialVisualizer() {
	cleanup();
}

bool RadialVisualizer::init(CCNode* parent) {
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

void RadialVisualizer::update(float dt, AudioFrame const& frame) {
	if (!m_drawNode) return;
	m_lastFrame = frame;
	m_hasFrame = true;
	const auto visualFrame = smoothVisualFrame(dt, frame);
	m_drawNode->clear();

	const auto winSize = CCDirector::sharedDirector()->getWinSize();
	const float centerX = winSize.width * 0.5f;
	const float centerY = winSize.height * 0.5f;
	const float minDimension = std::min(winSize.width, winSize.height);
	const float baseRadius = minDimension * kRadiusRatio;
	const float maximumAmplitude = std::min(kMaximumAmplitude, minDimension * kAmplitudeRatio);
	const bool reducedDetail = reduceVisualizerResourcesEnabled();
	const std::size_t segmentCount = reducedDetail
		? std::max<std::size_t>(1, (kAudioBandCount + 1) / 2)
		: kAudioBandCount;
	const bool transparencyEnabled = visualizerTransparencyEnabled();
	const ccColor4F baseColor = { 1.f, 1.f, 1.f,
		applyVisualizerOpacity(transparencyEnabled ? 0.95f : 1.f) };
	const float segmentRadius = std::clamp(minDimension * 0.004f, 2.5f, kSegmentRadius);

	for (std::size_t i = 0; i < segmentCount; ++i) {
		const auto [firstBand, endBand] = visualizerReducedRange(i, segmentCount, kAudioBandCount);
		float groupedEnergy = 0.f;
		for (std::size_t band = firstBand; band < endBand; ++band) {
			groupedEnergy += visualFrame.valid ? std::clamp(visualFrame.bands[band], 0.f, 1.f) : 0.f;
		}
		groupedEnergy /= static_cast<float>(std::max<std::size_t>(1, endBand - firstBand));
		const float energy = visualFrame.valid ? applyVisualizerPower(groupedEnergy) : 0.f;
		const float amplitude = kMinimumAmplitude + energy * (maximumAmplitude - kMinimumAmplitude);
		const float angle = -kPi * 0.5f + 2.f * kPi *
			(static_cast<float>(i) + 0.5f) / static_cast<float>(segmentCount);
		const CCPoint direction(std::cos(angle), std::sin(angle));
		const CCPoint inner(centerX + direction.x * baseRadius, centerY + direction.y * baseRadius);
		const float outerRadius = baseRadius + amplitude;
		const CCPoint outer(centerX + direction.x * outerRadius, centerY + direction.y * outerRadius);

		const auto configuredColor = visualizerColorAt(
			static_cast<float>(i) / static_cast<float>(std::max<std::size_t>(1, segmentCount - 1))
		);
		const ccColor4F color = {
			configuredColor.r / 255.f,
			configuredColor.g / 255.f,
			configuredColor.b / 255.f,
			baseColor.a,
		};
		m_drawNode->drawSegment(inner, outer, segmentRadius, color);
	}
}

void RadialVisualizer::cleanup() {
	setVisualizerOpacityChangedCallback({});
	setVisualizerPowerChangedCallback({});
	setVisualizerColorChangedCallback({});
	setReducedVisualizerResourcesChangedCallback({});
	if (!m_drawNode) return;
	m_drawNode->removeFromParentAndCleanup(true);
	m_drawNode->release();
	m_drawNode = nullptr;
}

cocos2d::CCRect RadialVisualizer::getBounds(cocos2d::CCSize winSize) const {
	const float radius = std::min(winSize.width, winSize.height) * kRadiusRatio
		+ std::min(kMaximumAmplitude, std::min(winSize.width, winSize.height) * kAmplitudeRatio)
		+ kSegmentRadius;
	const float centerX = winSize.width * 0.5f;
	const float centerY = winSize.height * 0.5f;
	return { centerX - radius, centerY - radius, radius * 2.f, radius * 2.f };
}

} // namespace geovision
