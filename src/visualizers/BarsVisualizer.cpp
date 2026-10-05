#include "BarsVisualizer.hpp"
#include "../config/VisualizerColor.hpp"
#include "../config/VisualizerOpacity.hpp"
#include "../config/VisualizerPower.hpp"

#include <Geode/Geode.hpp>

#include <algorithm>

using namespace geode::prelude;

namespace geovision {

namespace {
constexpr float kBarAreaWidthRatio = 0.52f;
constexpr float kBarAreaHeightRatio = 0.13f;
constexpr float kMinimumBarHeight = 10.f;
constexpr float kMaximumBarHeight = 72.f;
constexpr float kBarGap = 3.f;
} // namespace

BarsVisualizer::~BarsVisualizer() {
	cleanup();
}

bool BarsVisualizer::init(CCNode* parent) {
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
	log::info(
		"Geovision: draw node attached={}, visible={}, parentVisible={}",
		drawNode->getParent() == parent,
		drawNode->isVisible(),
		parent->isVisible()
	);
	return true;
}

void BarsVisualizer::update(float dt, AudioFrame const& frame) {
	if (!m_drawNode) return;
	m_lastFrame = frame;
	m_hasFrame = true;
	const auto visualFrame = smoothVisualFrame(dt, frame);
	m_drawNode->clear();

	const auto winSize = CCDirector::sharedDirector()->getWinSize();
	const float areaWidth = winSize.width * kBarAreaWidthRatio;
	const float areaHeight = std::min(kMaximumBarHeight, winSize.height * kBarAreaHeightRatio);
	const float startX = (winSize.width - areaWidth) * 0.5f;
	const float baseY = winSize.height * 0.08f;
	const bool transparencyEnabled = visualizerTransparencyEnabled();
	const bool reducedDetail = reduceVisualizerResourcesEnabled();
	const std::size_t barCount = reducedDetail ? (kAudioBandCount + 1) / 2 : kAudioBandCount;
	const float totalGap = kBarGap * static_cast<float>(barCount - 1);
	const float barWidth = std::max(1.f, (areaWidth - totalGap) / static_cast<float>(barCount));

	for (std::size_t i = 0; i < barCount; ++i) {
		const auto [firstBand, endBand] = reducedDetail
			? visualizerReducedRange(i, barCount, kAudioBandCount)
			: std::pair<std::size_t, std::size_t>{ i, i + 1 };
		float groupedEnergy = 0.f;
		for (std::size_t band = firstBand; band < endBand; ++band) {
			groupedEnergy += visualFrame.valid ? std::clamp(visualFrame.bands[band], 0.f, 1.f) : 0.f;
		}
		groupedEnergy /= static_cast<float>(std::max<std::size_t>(1, endBand - firstBand));
		const float colorPosition = static_cast<float>(firstBand + endBand - 1) /
			(2.f * static_cast<float>(kAudioBandCount - 1));
		const auto configuredColor = visualizerColorAt(colorPosition);
		const float red = configuredColor.r / 255.f;
		const float green = configuredColor.g / 255.f;
		const float blue = configuredColor.b / 255.f;
		const ccColor4F glowColor = { red * 0.5f, green * 0.5f, blue * 0.5f, applyVisualizerOpacity(transparencyEnabled ? 0.36f : 1.f) };
		const float energy = visualFrame.valid ? applyVisualizerPower(groupedEnergy) : 0.f;
		const float height = kMinimumBarHeight + energy * (areaHeight - kMinimumBarHeight);
		const float x = startX + static_cast<float>(i) * (barWidth + kBarGap);
		const CCPoint bottom(x, baseY);
		const CCPoint top(x + barWidth, baseY + height);
		m_drawNode->drawRect(bottom, top, glowColor, 0.f, glowColor);
	}

}

void BarsVisualizer::cleanup() {
	setVisualizerOpacityChangedCallback({});
	setVisualizerPowerChangedCallback({});
	setVisualizerColorChangedCallback({});
	setReducedVisualizerResourcesChangedCallback({});
	if (!m_drawNode) return;
	m_drawNode->removeFromParentAndCleanup(true);
	m_drawNode->release();
	m_drawNode = nullptr;
}

cocos2d::CCRect BarsVisualizer::getBounds(cocos2d::CCSize winSize) const {
	const float width = winSize.width * kBarAreaWidthRatio;
	const float height = std::min(kMaximumBarHeight, winSize.height * kBarAreaHeightRatio);
	return { (winSize.width - width) * 0.5f, winSize.height * 0.08f, width, height };
}

} // namespace geovision
