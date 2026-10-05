#pragma once

#include <Geode/Geode.hpp>

#include <algorithm>
#include <array>
#include <limits>

namespace geovision {

enum class VisualizerOrientation {
	Vertical,
	Horizontal,
};

struct VisualizerPositionState {
	float x = 0.f;
	float y = 0.f;
	float size = 1.f;
	VisualizerOrientation orientation = VisualizerOrientation::Vertical;
	cocos2d::CCNode* container = nullptr;
	cocos2d::CCRect visualizerBounds = { 0.f, 0.f, 0.f, 0.f };
	bool hasVisualizerBounds = false;
	cocos2d::CCDrawNode* guide = nullptr;
	cocos2d::CCScene* guideScene = nullptr;
};

inline VisualizerPositionState& visualizerPositionState() {
	static VisualizerPositionState state = [] {
		auto* mod = geode::Mod::get();
		return VisualizerPositionState{
			mod->getSavedValue<float>("visualizer-position-x", 0.f),
			mod->getSavedValue<float>("visualizer-position-y", 0.f),
			mod->getSavedValue<float>("visualizer-position-size", 1.f),
			mod->getSavedValue<int>("visualizer-orientation", 0) == 1
				? VisualizerOrientation::Horizontal
				: VisualizerOrientation::Vertical,
		};
	}();
	return state;
}

inline void updatePositionGuide() {
	auto& state = visualizerPositionState();
	if (!state.guide || !state.container || !state.guideScene || !state.hasVisualizerBounds) return;
	state.guide->clear();

	const auto& bounds = state.visualizerBounds;
	const float left = bounds.origin.x;
	const float right = bounds.origin.x + bounds.size.width;
	const float bottom = bounds.origin.y;
	const float top = bounds.origin.y + bounds.size.height;
	auto toGuideSpace = [&](cocos2d::CCPoint point) {
		return state.guideScene->convertToNodeSpace(state.container->convertToWorldSpace(point));
	};
	const auto bottomLeft = toGuideSpace({ left, bottom });
	const auto bottomRight = toGuideSpace({ right, bottom });
	const auto topRight = toGuideSpace({ right, top });
	const auto topLeft = toGuideSpace({ left, top });
	const cocos2d::ccColor4F color = { 1.f, 0.f, 0.f, 1.f };
	constexpr float thickness = 4.f;
	state.guide->drawSegment(bottomLeft, bottomRight, thickness, color);
	state.guide->drawSegment(bottomRight, topRight, thickness, color);
	state.guide->drawSegment(topRight, topLeft, thickness, color);
	state.guide->drawSegment(topLeft, bottomLeft, thickness, color);
}

inline void applyVisualizerPosition() {
	auto& state = visualizerPositionState();
	if (!state.container || !state.hasVisualizerBounds) return;
	state.x = std::clamp(state.x, -1.f, 1.f);
	state.y = std::clamp(state.y, -1.f, 1.f);

	const auto winSize = cocos2d::CCDirector::sharedDirector()->getWinSize();
	auto* scene = cocos2d::CCDirector::sharedDirector()->getRunningScene();
	auto* parent = state.container->getParent();
	if (!scene || !parent) return;

	// Convert the visible screen corners into the container parent's coordinates.
	const std::array screenCorners = {
		parent->convertToNodeSpace(scene->convertToWorldSpace(cocos2d::CCPointZero)),
		parent->convertToNodeSpace(scene->convertToWorldSpace({ winSize.width, 0.f })),
		parent->convertToNodeSpace(scene->convertToWorldSpace({ winSize.width, winSize.height })),
		parent->convertToNodeSpace(scene->convertToWorldSpace({ 0.f, winSize.height })),
	};
	float screenLeft = screenCorners[0].x;
	float screenRight = screenCorners[0].x;
	float screenBottom = screenCorners[0].y;
	float screenTop = screenCorners[0].y;
	for (auto const& corner : screenCorners) {
		screenLeft = std::min(screenLeft, corner.x);
		screenRight = std::max(screenRight, corner.x);
		screenBottom = std::min(screenBottom, corner.y);
		screenTop = std::max(screenTop, corner.y);
	}

	const auto contentSize = state.container->getContentSize();
	const auto anchor = state.container->getAnchorPoint();
	const float anchorX = contentSize.width * anchor.x;
	const float anchorY = contentSize.height * anchor.y;
	const auto& bounds = state.visualizerBounds;
	const bool rotated = state.orientation == VisualizerOrientation::Vertical;
	auto transformBoundsPoint = [&](float x, float y) {
		const float localX = x - anchorX;
		const float localY = y - anchorY;
		return rotated ? cocos2d::CCPoint(-localY, localX) : cocos2d::CCPoint(localX, localY);
	};
	const auto corners = std::array{
		transformBoundsPoint(bounds.origin.x, bounds.origin.y),
		transformBoundsPoint(bounds.origin.x + bounds.size.width, bounds.origin.y),
		transformBoundsPoint(bounds.origin.x + bounds.size.width, bounds.origin.y + bounds.size.height),
		transformBoundsPoint(bounds.origin.x, bounds.origin.y + bounds.size.height),
	};
	float localLeft = corners[0].x;
	float localRight = corners[0].x;
	float localBottom = corners[0].y;
	float localTop = corners[0].y;
	for (auto const& corner : corners) {
		localLeft = std::min(localLeft, corner.x);
		localRight = std::max(localRight, corner.x);
		localBottom = std::min(localBottom, corner.y);
		localTop = std::max(localTop, corner.y);
	}
	const float localWidth = localRight - localLeft;
	const float localHeight = localTop - localBottom;
	const float viewportWidth = screenRight - screenLeft;
	const float viewportHeight = screenTop - screenBottom;
	float effectiveSize = state.size;
	if (localWidth > 0.f && localHeight > 0.f) {
		effectiveSize = std::min(effectiveSize, std::min(viewportWidth / localWidth, viewportHeight / localHeight));
	}
	const float scaledLeft = localLeft * effectiveSize;
	const float scaledRight = localRight * effectiveSize;
	const float scaledBottom = localBottom * effectiveSize;
	const float scaledTop = localTop * effectiveSize;

	const float x = (state.x + 1.f) * 0.5f;
	const float y = (state.y + 1.f) * 0.5f;
	const float leftAlignedPosition = screenLeft - scaledLeft;
	const float rightAlignedPosition = screenRight - scaledRight;
	const float bottomAlignedPosition = screenBottom - scaledBottom;
	const float topAlignedPosition = screenTop - scaledTop;
	const float positionX = leftAlignedPosition + (rightAlignedPosition - leftAlignedPosition) * x;
	const float positionY = bottomAlignedPosition + (topAlignedPosition - bottomAlignedPosition) * y;

	state.container->setScale(effectiveSize);
	state.container->setRotation(rotated ? 90.f : 0.f);
	state.container->setPosition({ positionX, positionY });
	updatePositionGuide();
}

inline void attachVisualizerPositionContainer(cocos2d::CCNode* container) {
	auto& state = visualizerPositionState();
	state.container = container;
	applyVisualizerPosition();
}

inline void setVisualizerPositionBounds(cocos2d::CCRect bounds) {
	auto& state = visualizerPositionState();
	state.visualizerBounds = bounds;
	state.hasVisualizerBounds = true;
	applyVisualizerPosition();
}

inline void setVisualizerPositionPreview(bool editing) {
	auto& state = visualizerPositionState();
	if (!editing) {
		if (state.guide) state.guide->removeFromParentAndCleanup(true);
		state.guide = nullptr;
		state.guideScene = nullptr;
		return;
	}
	if (!state.container || !state.hasVisualizerBounds) return;
	auto* scene = cocos2d::CCDirector::sharedDirector()->getRunningScene();
	if (!scene) return;
	if (state.guide && state.guideScene != scene) {
		state.guide->removeFromParentAndCleanup(true);
		state.guide = nullptr;
		state.guideScene = nullptr;
	}
	if (!state.guide) {
		state.guide = cocos2d::CCDrawNode::create();
		if (!state.guide) return;
		state.guideScene = scene;
		state.guide->setAnchorPoint(cocos2d::CCPointZero);
		state.guide->setPosition(cocos2d::CCPointZero);
		state.guide->setContentSize(scene->getContentSize());
		state.guide->setVisible(true);
		state.guide->setOpacity(255);
		scene->addChild(state.guide, std::numeric_limits<int>::max());
	}
	updatePositionGuide();
}

inline void updateVisualizerPositionX(float value) {
	auto& state = visualizerPositionState();
	state.x = std::clamp(value, -1.f, 1.f);
	applyVisualizerPosition();
}

inline void updateVisualizerPositionY(float value) {
	auto& state = visualizerPositionState();
	state.y = std::clamp(value, -1.f, 1.f);
	applyVisualizerPosition();
}

inline void updateVisualizerPositionSize(float value) {
	auto& state = visualizerPositionState();
	state.size = std::clamp(value, 0.5f, 1.5f);
	applyVisualizerPosition();
}

inline void setVisualizerOrientation(VisualizerOrientation orientation) {
	auto& state = visualizerPositionState();
	state.orientation = orientation;
	geode::Mod::get()->setSavedValue(
		"visualizer-orientation",
		state.orientation == VisualizerOrientation::Horizontal ? 1 : 0
	);
	applyVisualizerPosition();
}

inline void saveVisualizerPosition() {
	const auto& state = visualizerPositionState();
	auto* mod = geode::Mod::get();
	mod->setSavedValue("visualizer-position-x", state.x);
	mod->setSavedValue("visualizer-position-y", state.y);
	mod->setSavedValue("visualizer-position-size", state.size);
}

inline void detachVisualizerPositionContainer() {
	auto& state = visualizerPositionState();
	setVisualizerPositionPreview(false);
	state.container = nullptr;
	state.hasVisualizerBounds = false;
}

} // namespace geovision
