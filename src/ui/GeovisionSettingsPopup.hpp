#pragma once

#include <Geode/Geode.hpp>

class GeovisionSettingsPopup final : public geode::Popup {
public:
	static GeovisionSettingsPopup* create();
	void onClose(cocos2d::CCObject* sender) override;

private:
	bool initPopup();
	void showWelcomeScreen();
	void onSectionSelected(cocos2d::CCObject* sender);
	void showVisualizerOptions();
	void showPositionOptions();
	void onOrientationSelected(cocos2d::CCObject* sender);
	void showColorOptions();
	void showOpacityOptions();
	void showPowerOptions();
	void showSensitivityOptions();
	void showSmoothingOptions();
	void showOptimizationOptions();
	void showPresetsOptions();
	void showMoreOptions();
	void onSavePreset(cocos2d::CCObject* sender);
	void onLoadPreset(cocos2d::CCObject* sender);
	void onDeletePreset(cocos2d::CCObject* sender);
	void onChooseVisualizerColor(cocos2d::CCObject* sender);
	void onChooseVisualizerColor2(cocos2d::CCObject* sender);
	void onTransparencyToggled(cocos2d::CCObject* sender);
	void onColor2Toggled(cocos2d::CCObject* sender);
	void onMatchBackgroundColorToggled(cocos2d::CCObject* sender);
	void onAvoidDuplicateRedrawToggled(cocos2d::CCObject* sender);
	void onReduceVisualizerResourcesToggled(cocos2d::CCObject* sender);
	void onBarsSelected(cocos2d::CCObject* sender);
	void onWaveSelected(cocos2d::CCObject* sender);
	void onRadialSelected(cocos2d::CCObject* sender);
	cocos2d::CCLayerColor* m_contentArea = nullptr;
	cocos2d::CCSprite* m_colorPreview = nullptr;
	cocos2d::CCSprite* m_color2Preview = nullptr;
	CCMenuItemSpriteExtra* m_transparencyToggle = nullptr;
	CCMenuItemSpriteExtra* m_color2Toggle = nullptr;
	CCMenuItemSpriteExtra* m_matchBackgroundColorToggle = nullptr;
	CCMenuItemSpriteExtra* m_avoidDuplicateRedrawToggle = nullptr;
	CCMenuItemSpriteExtra* m_reduceVisualizerResourcesToggle = nullptr;
	CCMenuItemSpriteExtra* m_color1Button = nullptr;
};
