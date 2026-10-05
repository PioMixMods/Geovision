#include "GeovisionSettingsPopup.hpp"

#include "../config/MatchBackgroundColor.hpp"
#include "../config/Optimization.hpp"
#include "../config/VisualizerPosition.hpp"
#include "../config/VisualizerColor.hpp"
#include "../config/VisualizerOpacity.hpp"
#include "../config/VisualizerPower.hpp"
#include "../config/VisualizerSensitivity.hpp"
#include "../config/VisualizerSmoothing.hpp"
#include "../config/VisualizerPresets.hpp"
#include "../visualizers/Visualizer.hpp"

#include <Geode/ui/ColorPickPopup.hpp>
#include <Geode/ui/SliderNode.hpp>
#include <Geode/ui/TextInput.hpp>

#include <array>
#include <cmath>
#include <string>
#include <vector>

using namespace geode::prelude;

namespace {
class GeovisionColorPickPopup final : public geode::ColorPickPopup {
public:
	GeovisionColorPickPopup() = default;

	static GeovisionColorPickPopup* create(
		cocos2d::ccColor3B color,
		std::function<void(cocos2d::ccColor3B)> callback
	) {
		auto* popup = new GeovisionColorPickPopup();
		popup->m_liveCallback = std::move(callback);
		if (popup->init({ color.r, color.g, color.b, 255 }, false)) {
			popup->autorelease();
			return popup;
		}
		delete popup;
		return nullptr;
	}

	void colorValueChanged(cocos2d::ccColor3B color) override {
		geode::ColorPickPopup::colorValueChanged(color);
		if (m_liveCallback) m_liveCallback(color);
	}

private:
	std::function<void(cocos2d::ccColor3B)> m_liveCallback;
};

class PresetNamePopup final : public geode::Popup {
public:
	static PresetNamePopup* create(std::function<void(std::string)> callback) {
		auto* popup = new PresetNamePopup();
		popup->m_callback = std::move(callback);
		if (popup->initPopup()) {
			popup->autorelease();
			return popup;
		}
		delete popup;
		return nullptr;
	}

private:
	bool initPopup() {
		if (!geode::Popup::init(320.f, 170.f)) return false;
		setTitle("Save Preset");
		m_input = geode::TextInput::create(250.f, "Preset name");
		if (!m_input) return false;
		m_input->setCommonFilter(geode::CommonFilter::Name);
		m_input->setMaxCharCount(32);
		m_input->setPosition({ m_mainLayer->getContentSize().width * 0.5f,
			m_mainLayer->getContentSize().height * 0.62f });
		m_mainLayer->addChild(m_input, 1);

		auto* menu = CCMenu::create();
		if (!menu) return false;
		menu->setPosition(CCPointZero);
		menu->setContentSize(m_mainLayer->getContentSize());
		m_mainLayer->addChild(menu, 2);
		auto addButton = [&](char const* label, SEL_MenuHandler handler, float x) {
			auto* sprite = ButtonSprite::create(label, 105.f, 0, 0.38f, true, "bigFont.fnt", "GJ_button_01.png", 24.f);
			if (!sprite) return;
			auto* button = CCMenuItemSpriteExtra::create(sprite, this, handler);
			if (!button) return;
			button->setPosition({ x, m_mainLayer->getContentSize().height * 0.25f });
			menu->addChild(button);
		};
		addButton("Cancel", menu_selector(PresetNamePopup::onCancel),
			m_mainLayer->getContentSize().width * 0.32f);
		addButton("Save", menu_selector(PresetNamePopup::onSave),
			m_mainLayer->getContentSize().width * 0.68f);
		return true;
	}

	void onCancel(CCObject*) {
		this->onClose(nullptr);
	}

	void onSave(CCObject*) {
		if (!m_input) return;
		const std::string name = geovision::sanitizeVisualizerPresetName(m_input->getString().c_str());
		if (name.empty()) {
			FLAlertLayer::create("Preset", "Enter a name for the preset.", "OK")->show();
			return;
		}
		if (geovision::hasVisualizerPreset(name)) {
			geode::createQuickPopup("Replace Preset", "A preset with this name already exists. Replace it?",
				"Cancel", "Replace", [this, name](FLAlertLayer*, bool confirmed) {
					if (confirmed) saveNamedPreset(name);
				});
			return;
		}
		saveNamedPreset(name);
	}

	void saveNamedPreset(std::string const& name) {
		geovision::saveVisualizerPreset(name);
		if (m_callback) m_callback(name);
		this->onClose(nullptr);
	}

	geode::TextInput* m_input = nullptr;
	std::function<void(std::string)> m_callback;
};

class PresetPickerPopup final : public geode::Popup {
public:
	static PresetPickerPopup* create(
		std::vector<std::string> names,
		bool deleting,
		std::function<void()> callback
	) {
		auto* popup = new PresetPickerPopup();
		popup->m_names = std::move(names);
		popup->m_selectedName = popup->m_names.empty() ? std::string{} : popup->m_names.front();
		popup->m_deleting = deleting;
		popup->m_callback = std::move(callback);
		if (popup->initPopup()) {
			popup->autorelease();
			return popup;
		}
		delete popup;
		return nullptr;
	}

private:
	bool initPopup() {
		if (!geode::Popup::init(350.f, 190.f)) return false;
		setTitle(m_deleting ? "Delete Preset" : "Load Preset");
		if (m_names.empty()) return true;
		const auto size = m_mainLayer->getContentSize();
		m_selectedLabel = CCLabelBMFont::create(m_selectedName.c_str(), "bigFont.fnt");
		if (m_selectedLabel) {
			m_selectedLabel->setPosition({ size.width * 0.5f, size.height * 0.62f });
			m_selectedLabel->limitLabelWidth(size.width - 100.f, 0.55f, 0.1f);
			m_mainLayer->addChild(m_selectedLabel, 1);
		}
		auto* menu = CCMenu::create();
		if (!menu) return false;
		menu->setPosition(CCPointZero);
		menu->setContentSize(size);
		m_mainLayer->addChild(menu, 2);
		auto* previousSprite = CCSprite::createWithSpriteFrameName("navArrowBtn_001.png");
		if (previousSprite) {
			previousSprite->setFlipX(true);
			previousSprite->setScale(0.35f);
			auto* previous = CCMenuItemSpriteExtra::create(
				previousSprite, this, menu_selector(PresetPickerPopup::onPrevious)
			);
			if (previous) {
				previous->setPosition({ 42.f, size.height * 0.62f });
				menu->addChild(previous);
			}
		}
		auto* nextSprite = CCSprite::createWithSpriteFrameName("navArrowBtn_001.png");
		if (nextSprite) {
			nextSprite->setScale(0.35f);
			auto* next = CCMenuItemSpriteExtra::create(
				nextSprite, this, menu_selector(PresetPickerPopup::onNext)
			);
			if (next) {
				next->setPosition({ size.width - 42.f, size.height * 0.62f });
				menu->addChild(next);
			}
		}
		auto* sprite = ButtonSprite::create(m_deleting ? "Delete" : "Load", 130.f, 0, 0.4f,
			true, "bigFont.fnt", "GJ_button_01.png", 26.f);
		if (sprite) {
			auto* button = CCMenuItemSpriteExtra::create(
				sprite, this, menu_selector(PresetPickerPopup::onConfirm)
			);
			if (button) {
				button->setPosition({ size.width * 0.5f, size.height * 0.25f });
				menu->addChild(button);
			}
		}
		return true;
	}

	void onConfirm(CCObject*) {
		if (m_selectedName.empty()) return;
		if (!m_deleting) {
			if (geovision::loadVisualizerPreset(m_selectedName) && m_callback) m_callback();
			this->onClose(nullptr);
			return;
		}
		const auto name = m_selectedName;
		geode::createQuickPopup("Delete Preset", "Delete preset \"" + name + "\"?",
			"Cancel", "Delete", [this, name](FLAlertLayer*, bool confirmed) {
				if (!confirmed) return;
				if (geovision::deleteVisualizerPreset(name) && m_callback) m_callback();
				this->onClose(nullptr);
			});
	}

	void updateSelectedName() {
		m_selectedName = m_names[m_selectedIndex];
		if (m_selectedLabel) {
			m_selectedLabel->setString(m_selectedName.c_str());
			m_selectedLabel->limitLabelWidth(m_mainLayer->getContentSize().width - 100.f, 0.55f, 0.1f);
		}
	}

	void onPrevious(CCObject*) {
		if (m_names.empty()) return;
		m_selectedIndex = (m_selectedIndex + m_names.size() - 1) % m_names.size();
		updateSelectedName();
	}

	void onNext(CCObject*) {
		if (m_names.empty()) return;
		m_selectedIndex = (m_selectedIndex + 1) % m_names.size();
		updateSelectedName();
	}

	std::vector<std::string> m_names;
	std::string m_selectedName;
	std::size_t m_selectedIndex = 0;
	CCLabelBMFont* m_selectedLabel = nullptr;
	bool m_deleting = false;
	std::function<void()> m_callback;
};
} // namespace

GeovisionSettingsPopup* GeovisionSettingsPopup::create() {
	auto* popup = new GeovisionSettingsPopup();
	if (popup && popup->initPopup()) {
		popup->autorelease();
		return popup;
	}
	delete popup;
	return nullptr;
}

bool GeovisionSettingsPopup::initPopup() {
	const auto windowSize = CCDirector::sharedDirector()->getWinSize();
	const float popupHeight = windowSize.height - 24.f;
	if (!geode::Popup::init(560.f, popupHeight)) {
		return false;
	}
	this->setTitle("Geovision");

	static constexpr char const* sections[] = {
		"Visualizer", "Position", "Color", "Opacity", "Power",
		"Sensitivity", "Smoothing", "Optimization", "Presets", "More",
	};
	const auto size = m_mainLayer->getContentSize();

	const float dividerX = size.width * 0.42f;
	m_contentArea = CCLayerColor::create(
		{ 12, 18, 34, 72 },
		size.width - dividerX - 28.f,
		size.height * 0.72f
	);
	if (m_contentArea) {
		m_contentArea->setPosition({ dividerX + 14.f, size.height * 0.14f });
		m_mainLayer->addChild(m_contentArea, 0);
	}
	auto* divider = CCLayerColor::create({ 110, 170, 220, 150 }, 2.f, size.height * 0.72f);
	if (divider) {
		divider->setPosition({ dividerX, size.height * 0.14f });
		m_mainLayer->addChild(divider, 1);
	}

	auto* menu = CCMenu::create();
	if (!menu) return true;
	menu->setContentSize(size);
	menu->setAnchorPoint({ 0.f, 0.f });
	menu->setPosition(CCPointZero);
	m_mainLayer->addChild(menu, 2);

	const float listBottom = size.height * 0.14f;
	const float listTop = size.height * 0.86f;
	constexpr float verticalMargin = 10.f;
	constexpr float horizontalGap = 16.f;
	constexpr float verticalGap = 14.f;
	const float buttonHeight = 24.f;
	const float availableHeight = listTop - listBottom - 2.f * verticalMargin;
	const std::size_t rowCount = (std::size(sections) + 1) / 2;
	const float rowStep = buttonHeight + verticalGap;
	const float listHeight = buttonHeight * static_cast<float>(rowCount)
		+ verticalGap * static_cast<float>(rowCount - 1);
	const float firstButtonY = listBottom + verticalMargin
		+ (availableHeight - listHeight) * 0.5f + buttonHeight * 0.5f;
	const float sideMargin = 14.f;
	const float buttonWidth = (dividerX - 2.f * sideMargin - horizontalGap) * 0.5f;
	const std::array<float, 2> columnCenters = {
		sideMargin + buttonWidth * 0.5f,
		sideMargin + buttonWidth + horizontalGap + buttonWidth * 0.5f,
	};
	for (std::size_t i = 0; i < std::size(sections); ++i) {
		auto* buttonSprite = ButtonSprite::create(
			sections[i], buttonWidth, 0, 0.38f, true, "bigFont.fnt", "GJ_button_01.png", 24.f
		);
		if (!buttonSprite) continue;
		auto* button = CCMenuItemSpriteExtra::create(
			buttonSprite,
			this,
			menu_selector(GeovisionSettingsPopup::onSectionSelected)
		);
		if (!button) continue;
		button->setTag(static_cast<int>(i));
		const std::size_t row = i / 2;
		const std::size_t column = i % 2;
		button->setPosition({ columnCenters[column], firstButtonY + static_cast<float>(row) * rowStep });
		menu->addChild(button);
	}
	showWelcomeScreen();
	return true;
}

void GeovisionSettingsPopup::showWelcomeScreen() {
	if (!m_contentArea) return;
	m_contentArea->removeAllChildrenWithCleanup(true);
	const auto size = m_contentArea->getContentSize();

	constexpr char titleText[] = "GEOVISION";
	constexpr std::array<cocos2d::ccColor3B, 4> gradientStops = {
		cocos2d::ccColor3B{ 66, 224, 255 },
		cocos2d::ccColor3B{ 92, 146, 255 },
		cocos2d::ccColor3B{ 180, 106, 255 },
		cocos2d::ccColor3B{ 255, 119, 196 },
	};
	constexpr float titleScale = 0.66f;
	constexpr float letterGap = 1.f;
	std::array<CCLabelBMFont*, sizeof(titleText) - 1> letters{};
	float titleWidth = letterGap * static_cast<float>(letters.size() - 1);
	for (std::size_t i = 0; i < letters.size(); ++i) {
		const char character[] = { titleText[i], '\0' };
		letters[i] = CCLabelBMFont::create(character, "bigFont.fnt");
		if (letters[i]) titleWidth += letters[i]->getContentSize().width * titleScale;
	}
	float cursorX = (size.width - titleWidth) * 0.5f;
	for (std::size_t i = 0; i < letters.size(); ++i) {
		auto* letter = letters[i];
		if (!letter) continue;
		const float t = static_cast<float>(i) / static_cast<float>(letters.size() - 1);
		const float stopPosition = t * static_cast<float>(gradientStops.size() - 1);
		const auto firstStop = std::min(static_cast<std::size_t>(stopPosition), gradientStops.size() - 2);
		const float blend = stopPosition - static_cast<float>(firstStop);
		const auto& first = gradientStops[firstStop];
		const auto& second = gradientStops[firstStop + 1];
		letter->setColor({
			static_cast<GLubyte>(std::lround(first.r + (second.r - first.r) * blend)),
			static_cast<GLubyte>(std::lround(first.g + (second.g - first.g) * blend)),
			static_cast<GLubyte>(std::lround(first.b + (second.b - first.b) * blend)),
		});
		letter->setScale(titleScale);
		letter->setPosition({ cursorX + letter->getContentSize().width * titleScale * 0.5f,
			size.height * 0.64f });
		m_contentArea->addChild(letter, 1);
		cursorX += letter->getContentSize().width * titleScale + letterGap;
	}

	auto* subtitle = CCLabelBMFont::create("Music Visualizer for Geometry Dash :)", "bigFont.fnt");
	if (subtitle) {
		subtitle->setScale(0.43f);
		subtitle->setColor({ 255, 207, 84 });
		subtitle->setPosition({ size.width * 0.5f, size.height * 0.49f });
		subtitle->limitLabelWidth(size.width * 0.92f, 0.43f, 0.25f);
		m_contentArea->addChild(subtitle, 1);
	}

	auto* instruction = CCLabelBMFont::create(
		"Select an option to configure your visualizer.", "bigFont.fnt"
	);
	if (instruction) {
		instruction->setScale(0.34f);
		instruction->setColor({ 218, 224, 235 });
		instruction->setPosition({ size.width * 0.5f, size.height * 0.39f });
		instruction->limitLabelWidth(size.width * 0.92f, 0.34f, 0.22f);
		m_contentArea->addChild(instruction, 1);
	}

	auto* version = CCLabelBMFont::create("v1.0.0", "bigFont.fnt");
	if (version) {
		version->setScale(0.28f);
		version->setColor({ 255, 255, 255 });
		version->setAnchorPoint({ 1.f, 1.f });
		version->setPosition({ size.width - 8.f, size.height - 8.f });
		m_contentArea->addChild(version, 1);
	}
}

void GeovisionSettingsPopup::onClose(CCObject* sender) {
	geovision::setVisualizerPositionPreview(false);
	geode::Popup::onClose(sender);
}

void GeovisionSettingsPopup::onSectionSelected(CCObject* sender) {
	auto* item = typeinfo_cast<CCNode*>(sender);
	if (!item) return;
	if (item->getTag() == 0) {
		showVisualizerOptions();
	} else if (item->getTag() == 1) {
		showPositionOptions();
	} else if (item->getTag() == 2) {
		showColorOptions();
	} else if (item->getTag() == 3) {
		showOpacityOptions();
	} else if (item->getTag() == 4) {
		showPowerOptions();
	} else if (item->getTag() == 5) {
		showSensitivityOptions();
	} else if (item->getTag() == 6) {
		showSmoothingOptions();
	} else if (item->getTag() == 7) {
		showOptimizationOptions();
	} else if (item->getTag() == 8) {
		showPresetsOptions();
	} else if (item->getTag() == 9) {
		showMoreOptions();
	}
}

void GeovisionSettingsPopup::showPresetsOptions() {
	if (!m_contentArea) return;
	m_contentArea->removeAllChildrenWithCleanup(true);
	const auto size = m_contentArea->getContentSize();
	if (auto* title = CCLabelBMFont::create("PRESETS", "bigFont.fnt")) {
		title->setScale(0.55f);
		title->setPosition({ size.width * 0.5f, size.height * 0.82f });
		m_contentArea->addChild(title, 1);
	}
	auto* menu = CCMenu::create();
	if (!menu) return;
	menu->setPosition(CCPointZero);
	menu->setContentSize(size);
	m_contentArea->addChild(menu, 2);
	auto addButton = [&](char const* label, SEL_MenuHandler handler, float y) {
		auto* sprite = ButtonSprite::create(label, 190.f, 0, 0.42f, true,
			"bigFont.fnt", "GJ_button_01.png", 28.f);
		if (!sprite) return;
		auto* button = CCMenuItemSpriteExtra::create(sprite, this, handler);
		if (!button) return;
		button->setPosition({ size.width * 0.5f, y });
		menu->addChild(button);
	};
	addButton("Save Preset", menu_selector(GeovisionSettingsPopup::onSavePreset), size.height * 0.62f);
	addButton("Load Preset", menu_selector(GeovisionSettingsPopup::onLoadPreset), size.height * 0.42f);
	addButton("Delete Preset", menu_selector(GeovisionSettingsPopup::onDeletePreset), size.height * 0.22f);
}

void GeovisionSettingsPopup::onSavePreset(CCObject*) {
	if (auto* popup = PresetNamePopup::create([this](std::string const&) { showPresetsOptions(); })) {
		popup->show();
	}
}

void GeovisionSettingsPopup::onLoadPreset(CCObject*) {
	auto names = geovision::visualizerPresetNames();
	if (names.empty()) {
		FLAlertLayer::create("Presets", "There are no saved presets.", "OK")->show();
		return;
	}
	if (auto* popup = PresetPickerPopup::create(std::move(names), false,
		[this] { showPresetsOptions(); })) {
		popup->show();
	}
}

void GeovisionSettingsPopup::onDeletePreset(CCObject*) {
	auto names = geovision::visualizerPresetNames();
	if (names.empty()) {
		FLAlertLayer::create("Presets", "There are no saved presets.", "OK")->show();
		return;
	}
	if (auto* popup = PresetPickerPopup::create(std::move(names), true,
		[this] { showPresetsOptions(); })) {
		popup->show();
	}
}

void GeovisionSettingsPopup::showOptimizationOptions() {
	if (!m_contentArea) return;
	m_contentArea->removeAllChildrenWithCleanup(true);
	m_avoidDuplicateRedrawToggle = nullptr;
	m_reduceVisualizerResourcesToggle = nullptr;

	const auto size = m_contentArea->getContentSize();
	auto* title = CCLabelBMFont::create("OPTIMIZATION", "bigFont.fnt");
	if (title) {
		title->setScale(0.55f);
		title->setPosition({ size.width * 0.5f, size.height * 0.82f });
		m_contentArea->addChild(title, 1);
	}

	auto* label = CCLabelBMFont::create("Avoid Duplicate Redraw", "bigFont.fnt");
	if (label) {
		label->setScale(0.36f);
		label->setAnchorPoint({ 0.f, 0.5f });
		label->setPosition({ size.width * 0.08f, size.height * 0.69f });
		m_contentArea->addChild(label, 1);
	}

	auto* menu = CCMenu::create();
	if (!menu) return;
	menu->setContentSize(size);
	menu->setAnchorPoint(CCPointZero);
	menu->setPosition(CCPointZero);
	m_contentArea->addChild(menu, 2);

	auto* toggleSprite = CCSprite::createWithSpriteFrameName(
		geovision::avoidDuplicateRedrawEnabled() ? "GJ_checkOn_001.png" : "GJ_checkOff_001.png"
	);
	if (toggleSprite) {
		toggleSprite->setScale(0.7f);
		m_avoidDuplicateRedrawToggle = CCMenuItemSpriteExtra::create(
			toggleSprite, this, menu_selector(GeovisionSettingsPopup::onAvoidDuplicateRedrawToggled)
		);
		if (m_avoidDuplicateRedrawToggle) {
			m_avoidDuplicateRedrawToggle->setPosition({ size.width * 0.9f, size.height * 0.69f });
			menu->addChild(m_avoidDuplicateRedrawToggle);
		}
	}

	auto* info = CCLabelBMFont::create(
		"Only works when Match Background Color is enabled.",
		"bigFont.fnt"
	);
	if (info) {
		info->setScale(0.30f);
		info->setAlignment(CCTextAlignment::kCCTextAlignmentCenter);
		info->setPosition({ size.width * 0.5f, size.height * 0.57f });
		m_contentArea->addChild(info, 1);
	}

	auto* reducedLabel = CCLabelBMFont::create("Reduce Visualizer Resources", "bigFont.fnt");
	if (reducedLabel) {
		reducedLabel->setScale(0.36f);
		reducedLabel->setAnchorPoint({ 0.f, 0.5f });
		reducedLabel->setPosition({ size.width * 0.08f, size.height * 0.38f });
		m_contentArea->addChild(reducedLabel, 1);
	}

	if (auto* toggleSprite = CCSprite::createWithSpriteFrameName(
		geovision::reduceVisualizerResourcesEnabled() ? "GJ_checkOn_001.png" : "GJ_checkOff_001.png"
	)) {
		toggleSprite->setScale(0.7f);
		m_reduceVisualizerResourcesToggle = CCMenuItemSpriteExtra::create(
			toggleSprite, this, menu_selector(GeovisionSettingsPopup::onReduceVisualizerResourcesToggled)
		);
		if (m_reduceVisualizerResourcesToggle) {
			m_reduceVisualizerResourcesToggle->setPosition({ size.width * 0.9f, size.height * 0.38f });
			menu->addChild(m_reduceVisualizerResourcesToggle);
		}
	}

	auto* reducedInfo = CCLabelBMFont::create(
		"Reduces visual detail to improve performance.", "bigFont.fnt"
	);
	if (reducedInfo) {
		reducedInfo->setScale(0.30f);
		reducedInfo->setAlignment(CCTextAlignment::kCCTextAlignmentCenter);
		reducedInfo->setPosition({ size.width * 0.5f, size.height * 0.23f });
		m_contentArea->addChild(reducedInfo, 1);
	}
}

void GeovisionSettingsPopup::onAvoidDuplicateRedrawToggled(CCObject* sender) {
	if (!typeinfo_cast<CCMenuItemSpriteExtra*>(sender)) return;
	const bool enabled = !geovision::avoidDuplicateRedrawEnabled();
	geovision::setAvoidDuplicateRedrawEnabled(enabled);
	if (m_avoidDuplicateRedrawToggle) {
		if (auto* sprite = CCSprite::createWithSpriteFrameName(
			enabled ? "GJ_checkOn_001.png" : "GJ_checkOff_001.png"
		)) {
			sprite->setScale(0.7f);
			m_avoidDuplicateRedrawToggle->setSprite(sprite);
		}
	}
}

void GeovisionSettingsPopup::onReduceVisualizerResourcesToggled(CCObject* sender) {
	if (!typeinfo_cast<CCMenuItemSpriteExtra*>(sender)) return;
	const bool enabled = !geovision::reduceVisualizerResourcesEnabled();
	geovision::setReduceVisualizerResourcesEnabled(enabled);
	if (m_reduceVisualizerResourcesToggle) {
		if (auto* sprite = CCSprite::createWithSpriteFrameName(
			enabled ? "GJ_checkOn_001.png" : "GJ_checkOff_001.png"
		)) {
			sprite->setScale(0.7f);
			m_reduceVisualizerResourcesToggle->setSprite(sprite);
		}
	}
}

void GeovisionSettingsPopup::showMoreOptions() {
	if (!m_contentArea) return;
	m_contentArea->removeAllChildrenWithCleanup(true);
	m_matchBackgroundColorToggle = nullptr;
	m_color1Button = nullptr;

	const auto size = m_contentArea->getContentSize();
	auto* title = CCLabelBMFont::create("MORE", "bigFont.fnt");
	if (title) {
		title->setScale(0.55f);
		title->setPosition({ size.width * 0.5f, size.height * 0.82f });
		m_contentArea->addChild(title, 1);
	}

	auto* label = CCLabelBMFont::create("Match Background Color", "bigFont.fnt");
	if (label) {
		label->setScale(0.4f);
		label->setAnchorPoint({ 0.f, 0.5f });
		label->setPosition({ size.width * 0.12f, size.height * 0.62f });
		m_contentArea->addChild(label, 1);
	}

	auto* menu = CCMenu::create();
	if (!menu) return;
	menu->setContentSize(size);
	menu->setAnchorPoint(CCPointZero);
	menu->setPosition(CCPointZero);
	m_contentArea->addChild(menu, 2);

	auto* toggleSprite = CCSprite::createWithSpriteFrameName(
		geovision::matchBackgroundColorEnabled() ? "GJ_checkOn_001.png" : "GJ_checkOff_001.png"
	);
	if (toggleSprite) {
		toggleSprite->setScale(0.7f);
		m_matchBackgroundColorToggle = CCMenuItemSpriteExtra::create(
			toggleSprite, this, menu_selector(GeovisionSettingsPopup::onMatchBackgroundColorToggled)
		);
		if (m_matchBackgroundColorToggle) {
			m_matchBackgroundColorToggle->setPosition({ size.width * 0.86f, size.height * 0.62f });
			menu->addChild(m_matchBackgroundColorToggle);
		}
	}

	auto* warning = CCLabelBMFont::create("Visualizer color follows the level Background.", "bigFont.fnt");
	if (warning) {
		warning->setScale(0.30f);
		warning->setAlignment(CCTextAlignment::kCCTextAlignmentCenter);
		warning->setPosition({ size.width * 0.5f, size.height * 0.4f });
		m_contentArea->addChild(warning, 1);
	}
}


void GeovisionSettingsPopup::onMatchBackgroundColorToggled(CCObject* sender) {
	if (!typeinfo_cast<CCMenuItemSpriteExtra*>(sender)) return;
	const bool enabled = !geovision::matchBackgroundColorEnabled();
	geovision::setMatchBackgroundColorEnabled(enabled);
	if (m_color1Button) {
		m_color1Button->setEnabled(!enabled);
		m_color1Button->setOpacity(enabled ? 120 : 255);
	}
	if (m_matchBackgroundColorToggle) {
		if (auto* sprite = CCSprite::createWithSpriteFrameName(
			enabled ? "GJ_checkOn_001.png" : "GJ_checkOff_001.png"
		)) {
			sprite->setScale(0.7f);
			m_matchBackgroundColorToggle->setSprite(sprite);
		}
	}
}

void GeovisionSettingsPopup::showSmoothingOptions() {
	if (!m_contentArea) return;
	m_contentArea->removeAllChildrenWithCleanup(true);

	const auto size = m_contentArea->getContentSize();
	auto* title = CCLabelBMFont::create("SMOOTHING", "bigFont.fnt");
	if (title) {
		title->setScale(0.55f);
		title->setPosition({ size.width * 0.5f, size.height * 0.78f });
		m_contentArea->addChild(title, 1);
	}

	auto* valueLabel = CCLabelBMFont::create("", "bigFont.fnt");
	if (valueLabel) {
		valueLabel->setScale(0.45f);
		valueLabel->setPosition({ size.width * 0.5f, size.height * 0.58f });
		m_contentArea->addChild(valueLabel, 1);
	}

	auto* slider = geode::SliderNode::create([](geode::SliderNode*, float) {});
	if (!slider) return;
	slider->setMin(0.f);
	slider->setMax(100.f);
	const float initialSmoothing = geovision::visualizerSmoothing();
	slider->setValue(initialSmoothing);
	slider->setContentSize({ size.width - 52.f, 22.f });
	slider->setPosition({ size.width * 0.5f, size.height * 0.42f });
	m_contentArea->addChild(slider, 2);

	auto updateLabel = [valueLabel](float smoothing) {
		if (!valueLabel) return;
		const auto text = std::to_string(static_cast<int>(std::lround(smoothing))) + "%";
		valueLabel->setString(text.c_str());
	};
	updateLabel(initialSmoothing);
	slider->setSlideCallback([updateLabel](geode::SliderNode*, float smoothing) {
		geovision::setVisualizerSmoothing(smoothing);
		updateLabel(geovision::visualizerSmoothing());
	});
}

void GeovisionSettingsPopup::showSensitivityOptions() {
	if (!m_contentArea) return;
	m_contentArea->removeAllChildrenWithCleanup(true);

	const auto size = m_contentArea->getContentSize();
	auto* title = CCLabelBMFont::create("SENSITIVITY", "bigFont.fnt");
	if (title) {
		title->setScale(0.55f);
		title->setPosition({ size.width * 0.5f, size.height * 0.78f });
		m_contentArea->addChild(title, 1);
	}

	auto* valueLabel = CCLabelBMFont::create("", "bigFont.fnt");
	if (valueLabel) {
		valueLabel->setScale(0.45f);
		valueLabel->setPosition({ size.width * 0.5f, size.height * 0.58f });
		m_contentArea->addChild(valueLabel, 1);
	}

	auto* slider = geode::SliderNode::create([](geode::SliderNode*, float) {});
	if (!slider) return;
	slider->setMin(0.f);
	slider->setMax(200.f);
	const float initialSensitivity = geovision::visualizerSensitivity();
	slider->setValue(initialSensitivity);
	slider->setContentSize({ size.width - 52.f, 22.f });
	slider->setPosition({ size.width * 0.5f, size.height * 0.42f });
	m_contentArea->addChild(slider, 2);

	auto updateLabel = [valueLabel](float sensitivity) {
		if (!valueLabel) return;
		const auto text = std::to_string(static_cast<int>(std::lround(sensitivity))) + "%";
		valueLabel->setString(text.c_str());
	};
	updateLabel(initialSensitivity);
	slider->setSlideCallback([updateLabel](geode::SliderNode*, float sensitivity) {
		geovision::setVisualizerSensitivity(sensitivity);
		updateLabel(geovision::visualizerSensitivity());
	});
}

void GeovisionSettingsPopup::showPowerOptions() {
	if (!m_contentArea) return;
	m_contentArea->removeAllChildrenWithCleanup(true);

	const auto size = m_contentArea->getContentSize();
	auto* title = CCLabelBMFont::create("POWER", "bigFont.fnt");
	if (title) {
		title->setScale(0.55f);
		title->setPosition({ size.width * 0.5f, size.height * 0.78f });
		m_contentArea->addChild(title, 1);
	}

	auto* valueLabel = CCLabelBMFont::create("", "bigFont.fnt");
	if (valueLabel) {
		valueLabel->setScale(0.45f);
		valueLabel->setPosition({ size.width * 0.5f, size.height * 0.58f });
		m_contentArea->addChild(valueLabel, 1);
	}

	auto* slider = geode::SliderNode::create([](geode::SliderNode*, float) {});
	if (!slider) return;
	slider->setMin(0.f);
	slider->setMax(200.f);
	const float initialPower = geovision::visualizerPower();
	slider->setValue(initialPower);
	slider->setContentSize({ size.width - 52.f, 22.f });
	slider->setPosition({ size.width * 0.5f, size.height * 0.42f });
	m_contentArea->addChild(slider, 2);

	auto updateLabel = [valueLabel](float power) {
		if (!valueLabel) return;
		const auto text = std::to_string(static_cast<int>(std::lround(power))) + "%";
		valueLabel->setString(text.c_str());
	};
	updateLabel(initialPower);
	slider->setSlideCallback([updateLabel](geode::SliderNode*, float power) {
		geovision::setVisualizerPower(power);
		updateLabel(geovision::visualizerPower());
	});
}

void GeovisionSettingsPopup::showOpacityOptions() {
	if (!m_contentArea) return;
	m_contentArea->removeAllChildrenWithCleanup(true);

	const auto size = m_contentArea->getContentSize();
	auto* title = CCLabelBMFont::create("OPACITY", "bigFont.fnt");
	if (title) {
		title->setScale(0.55f);
		title->setPosition({ size.width * 0.5f, size.height * 0.78f });
		m_contentArea->addChild(title, 1);
	}

	auto* valueLabel = CCLabelBMFont::create("", "bigFont.fnt");
	if (valueLabel) {
		valueLabel->setScale(0.45f);
		valueLabel->setPosition({ size.width * 0.5f, size.height * 0.58f });
		m_contentArea->addChild(valueLabel, 1);
	}

	auto* slider = geode::SliderNode::create([](geode::SliderNode*, float) {});
	if (!slider) return;
	slider->setMin(0.f);
	slider->setMax(100.f);
	const float initialOpacity = geovision::visualizerOpacity();
	slider->setValue(initialOpacity);
	slider->setContentSize({ size.width - 52.f, 22.f });
	slider->setPosition({ size.width * 0.5f, size.height * 0.42f });
	m_contentArea->addChild(slider, 2);

	auto updateLabel = [valueLabel](float opacity) {
		if (!valueLabel) return;
		const auto text = std::to_string(static_cast<int>(std::lround(opacity))) + "%";
		valueLabel->setString(text.c_str());
	};
	updateLabel(initialOpacity);
	slider->setSlideCallback([updateLabel](geode::SliderNode*, float opacity) {
		geovision::setVisualizerOpacity(opacity);
		updateLabel(geovision::visualizerOpacity());
	});
}

void GeovisionSettingsPopup::showColorOptions() {
	if (!m_contentArea) return;
	m_contentArea->removeAllChildrenWithCleanup(true);
	m_color1Button = nullptr;
	m_colorPreview = nullptr;
	m_color2Preview = nullptr;
	m_transparencyToggle = nullptr;
	m_color2Toggle = nullptr;

	const auto size = m_contentArea->getContentSize();
	auto* title = CCLabelBMFont::create("COLOR", "bigFont.fnt");
	if (title) {
		title->setScale(0.55f);
		title->setPosition({ size.width * 0.5f, size.height * 0.88f });
		m_contentArea->addChild(title, 1);
	}

	const bool matchBackgroundColor = geovision::matchBackgroundColorEnabled();
	auto* color1Label = CCLabelBMFont::create("Color 1", "bigFont.fnt");
	if (color1Label) {
		color1Label->setScale(0.4f);
		color1Label->setAnchorPoint({ 0.f, 0.5f });
		color1Label->setPosition({ size.width * 0.18f, size.height * 0.77f });
		if (matchBackgroundColor) color1Label->setOpacity(120);
		m_contentArea->addChild(color1Label, 1);
	}
	m_colorPreview = CCSprite::createWithSpriteFrameName("whiteSquare60_001.png");
	if (m_colorPreview) {
		m_colorPreview->setColor(geovision::visualizerColor());
		m_colorPreview->setScale(0.55f);
		m_colorPreview->setPosition({ size.width * 0.23f, size.height * 0.64f });
		if (matchBackgroundColor) m_colorPreview->setOpacity(120);
		m_contentArea->addChild(m_colorPreview, 1);
	}
	auto* color2Label = CCLabelBMFont::create("Color 2", "bigFont.fnt");
	if (color2Label) {
		color2Label->setScale(0.4f);
		color2Label->setAnchorPoint({ 0.f, 0.5f });
		color2Label->setPosition({ size.width * 0.18f, size.height * 0.46f });
		m_contentArea->addChild(color2Label, 1);
	}
	m_color2Preview = CCSprite::createWithSpriteFrameName("whiteSquare60_001.png");
	if (m_color2Preview) {
		m_color2Preview->setColor(geovision::visualizerColor2());
		m_color2Preview->setScale(0.55f);
		m_color2Preview->setPosition({ size.width * 0.23f, size.height * 0.33f });
		m_contentArea->addChild(m_color2Preview, 1);
	}

	auto* menu = CCMenu::create();
	if (!menu) return;
	menu->setContentSize(size);
	menu->setAnchorPoint(CCPointZero);
	menu->setPosition(CCPointZero);
	m_contentArea->addChild(menu, 2);

	auto addColorButton = [&](char const* text, SEL_MenuHandler callback, float y, bool enabled) -> CCMenuItemSpriteExtra* {
		auto* sprite = ButtonSprite::create(text, 150, 0, 0.4f, true, "bigFont.fnt", "GJ_button_01.png", 30.f);
		if (!sprite) return nullptr;
		auto* button = CCMenuItemSpriteExtra::create(sprite, this, callback);
		if (!button) return nullptr;
		button->setPosition({ size.width * 0.68f, y });
		button->setEnabled(enabled);
		button->setOpacity(enabled ? 255 : 120);
		menu->addChild(button);
		return button;
	};
	m_color1Button = addColorButton(
		"Choose Color 1", menu_selector(GeovisionSettingsPopup::onChooseVisualizerColor),
		size.height * 0.64f, !matchBackgroundColor
	);
	addColorButton(
		"Choose Color 2", menu_selector(GeovisionSettingsPopup::onChooseVisualizerColor2),
		size.height * 0.33f, true
	);

	auto* useColor2Label = CCLabelBMFont::create("Use Color 2", "bigFont.fnt");
	if (useColor2Label) {
		useColor2Label->setScale(0.34f);
		useColor2Label->setAnchorPoint({ 0.f, 0.5f });
		useColor2Label->setPosition({ size.width * 0.08f, size.height * 0.11f });
		m_contentArea->addChild(useColor2Label, 1);
	}
	auto* color2ToggleSprite = CCSprite::createWithSpriteFrameName(
		geovision::visualizerColor2Enabled() ? "GJ_checkOn_001.png" : "GJ_checkOff_001.png"
	);
	if (color2ToggleSprite) {
		color2ToggleSprite->setScale(0.62f);
		m_color2Toggle = CCMenuItemSpriteExtra::create(
			color2ToggleSprite, this, menu_selector(GeovisionSettingsPopup::onColor2Toggled)
		);
		if (m_color2Toggle) {
			m_color2Toggle->setPosition({ size.width * 0.46f, size.height * 0.11f });
			menu->addChild(m_color2Toggle);
		}
	}
	auto* transparencyLabel = CCLabelBMFont::create("Transparency", "bigFont.fnt");
	if (transparencyLabel) {
		transparencyLabel->setScale(0.34f);
		transparencyLabel->setAnchorPoint({ 0.f, 0.5f });
		transparencyLabel->setPosition({ size.width * 0.58f, size.height * 0.11f });
		m_contentArea->addChild(transparencyLabel, 1);
	}
	auto* toggleSprite = CCSprite::createWithSpriteFrameName(
		geovision::visualizerTransparencyEnabled() ? "GJ_checkOn_001.png" : "GJ_checkOff_001.png"
	);
	if (!toggleSprite) return;
	toggleSprite->setScale(0.62f);
	m_transparencyToggle = CCMenuItemSpriteExtra::create(
		toggleSprite,
		this,
		menu_selector(GeovisionSettingsPopup::onTransparencyToggled)
	);
	if (m_transparencyToggle) {
		m_transparencyToggle->setPosition({ size.width * 0.93f, size.height * 0.11f });
		menu->addChild(m_transparencyToggle);
	}
}

void GeovisionSettingsPopup::onChooseVisualizerColor2(CCObject*) {
	auto applyColor = [this](cocos2d::ccColor3B color) {
		geovision::setVisualizerColor2(color);
		if (m_color2Preview) m_color2Preview->setColor(color);
	};
	auto* picker = GeovisionColorPickPopup::create(geovision::visualizerColor2(), applyColor);
	if (!picker) return;
	picker->setCallback([applyColor](cocos2d::ccColor4B const& color) {
		applyColor({ color.r, color.g, color.b });
	});
	picker->show();
}

void GeovisionSettingsPopup::onChooseVisualizerColor(CCObject*) {
	if (geovision::matchBackgroundColorEnabled()) return;
	auto applyColor = [this](cocos2d::ccColor3B color) {
		geovision::setVisualizerColor(color);
		if (m_colorPreview) m_colorPreview->setColor(color);
	};
	auto* picker = GeovisionColorPickPopup::create(geovision::visualizerColor(), applyColor);
	if (!picker) return;
	picker->setCallback([applyColor](cocos2d::ccColor4B const& color) {
		applyColor({ color.r, color.g, color.b });
	});
	picker->show();
}

void GeovisionSettingsPopup::onTransparencyToggled(CCObject* sender) {
	if (!typeinfo_cast<CCMenuItemSpriteExtra*>(sender)) return;
	const bool enabled = !geovision::visualizerTransparencyEnabled();
	geovision::setVisualizerTransparencyEnabled(enabled);
	if (m_transparencyToggle) {
		if (auto* sprite = CCSprite::createWithSpriteFrameName(
			enabled ? "GJ_checkOn_001.png" : "GJ_checkOff_001.png"
		)) {
			sprite->setScale(0.7f);
			m_transparencyToggle->setSprite(sprite);
		}
	}
}

void GeovisionSettingsPopup::onColor2Toggled(CCObject* sender) {
	if (!typeinfo_cast<CCMenuItemSpriteExtra*>(sender)) return;
	const bool enabled = !geovision::visualizerColor2Enabled();
	geovision::setVisualizerColor2Enabled(enabled);
	if (m_color2Toggle) {
		if (auto* sprite = CCSprite::createWithSpriteFrameName(
			enabled ? "GJ_checkOn_001.png" : "GJ_checkOff_001.png"
		)) {
			sprite->setScale(0.62f);
			m_color2Toggle->setSprite(sprite);
		}
	}
}

void GeovisionSettingsPopup::showVisualizerOptions() {
	if (!m_contentArea) return;
	m_contentArea->removeAllChildrenWithCleanup(true);

	const auto size = m_contentArea->getContentSize();
	auto* title = CCLabelBMFont::create("VISUALIZER", "bigFont.fnt");
	if (title) {
		title->setScale(0.55f);
		title->setPosition({ size.width * 0.5f, size.height * 0.82f });
		m_contentArea->addChild(title, 1);
	}

	auto* menu = CCMenu::create();
	if (!menu) return;
	menu->setContentSize(size);
	menu->setAnchorPoint(CCPointZero);
	menu->setPosition(CCPointZero);
	m_contentArea->addChild(menu, 2);

	const auto selected = geovision::g_selectedVisualizerType;
	auto addChoice = [&](char const* text, SEL_MenuHandler callback, float y) {
		auto* sprite = ButtonSprite::create(text, 180, 0, 0.45f, true, "bigFont.fnt", "GJ_button_01.png", 34.f);
		if (!sprite) return;
		auto* button = CCMenuItemSpriteExtra::create(sprite, this, callback);
		if (!button) return;
		button->setPosition({ size.width * 0.5f, y });
		menu->addChild(button);
	};

	addChoice(selected == geovision::VisualizerType::Bars ? "[X] Bars" : "[ ] Bars",
		menu_selector(GeovisionSettingsPopup::onBarsSelected), size.height * 0.59f);
	addChoice(selected == geovision::VisualizerType::Wave ? "[X] Wave" : "[ ] Wave",
		menu_selector(GeovisionSettingsPopup::onWaveSelected), size.height * 0.39f);
	addChoice(selected == geovision::VisualizerType::Radial ? "[X] Radial" : "[ ] Radial",
		menu_selector(GeovisionSettingsPopup::onRadialSelected), size.height * 0.19f);
}

void GeovisionSettingsPopup::onBarsSelected(CCObject*) {
	geovision::g_selectedVisualizerType = geovision::VisualizerType::Bars;
	showVisualizerOptions();
}

void GeovisionSettingsPopup::onWaveSelected(CCObject*) {
	geovision::g_selectedVisualizerType = geovision::VisualizerType::Wave;
	showVisualizerOptions();
}

void GeovisionSettingsPopup::onRadialSelected(CCObject*) {
	geovision::g_selectedVisualizerType = geovision::VisualizerType::Radial;
	showVisualizerOptions();
}

void GeovisionSettingsPopup::showPositionOptions() {
	if (!m_contentArea) return;
	m_contentArea->removeAllChildrenWithCleanup(true);

	const auto size = m_contentArea->getContentSize();
	auto* title = CCLabelBMFont::create("POSITION", "bigFont.fnt");
	if (title) {
		title->setScale(0.55f);
		title->setPosition({ size.width * 0.5f, size.height * 0.88f });
		m_contentArea->addChild(title, 1);
	}
	auto* menu = CCMenu::create();
	if (!menu) return;
	menu->setContentSize(size);
	menu->setAnchorPoint(CCPointZero);
	menu->setPosition(CCPointZero);
	m_contentArea->addChild(menu, 2);

	const auto& state = geovision::visualizerPositionState();
	auto addSliderRow = [&](char const* labelText, float rowY, float value, float minimum, float maximum,
	std::function<void(float)> applyValue) {
		auto* label = CCLabelBMFont::create(labelText, "bigFont.fnt");
		if (label) {
			label->setScale(0.4f);
			label->setAnchorPoint({ 0.f, 0.5f });
			label->setPosition({ 20.f, rowY });
			m_contentArea->addChild(label, 1);
		}
		auto* valueLabel = CCLabelBMFont::create("", "bigFont.fnt");
		if (valueLabel) {
			valueLabel->setScale(0.36f);
			valueLabel->setAnchorPoint({ 1.f, 0.5f });
			valueLabel->setPosition({ size.width - 20.f, rowY });
			m_contentArea->addChild(valueLabel, 1);
		}

		auto* slider = geode::SliderNode::create([](geode::SliderNode*, float) {});
		if (!slider) return;
		slider->setMin(minimum);
		slider->setMax(maximum);
		slider->setValue(value);
		slider->setContentSize({ size.width - 52.f, 22.f });
		slider->setPosition({ size.width * 0.5f, rowY - 22.f });
		m_contentArea->addChild(slider, 2);

		auto updateValueLabel = [valueLabel](float current) {
			if (!valueLabel) return;
			const auto text = std::to_string(static_cast<int>(std::lround(current))) + "%";
			valueLabel->setString(text.c_str());
		};
		updateValueLabel(value);
		slider->setSlideCallback([applyValue, updateValueLabel](geode::SliderNode*, float current) {
			applyValue(current);
			updateValueLabel(current);
			geovision::setVisualizerPositionPreview(true);
		});
		slider->setClickCallback([](geode::SliderNode*, float) {
			geovision::setVisualizerPositionPreview(true);
		});
		slider->setReleaseCallback([](geode::SliderNode*, float) {
			geovision::saveVisualizerPosition();
			geovision::setVisualizerPositionPreview(false);
		});
	};

	addSliderRow("Pos X", size.height * 0.82f, state.x * 100.f, -100.f, 100.f,
		[](float value) { geovision::updateVisualizerPositionX(value / 100.f); });
	addSliderRow("Pos Y", size.height * 0.59f, state.y * 100.f, -100.f, 100.f,
		[](float value) { geovision::updateVisualizerPositionY(value / 100.f); });
	addSliderRow("Size", size.height * 0.36f, state.size * 100.f, 50.f, 150.f,
		[](float value) { geovision::updateVisualizerPositionSize(value / 100.f); });

	const float orientationButtonY = std::max(12.f, size.height * 0.07f);
	const float sizeSliderBottom = size.height * 0.32f - 39.f;
	const float orientationButtonTop = orientationButtonY + 10.f;
	const float orientationLabelY = std::max(
		orientationButtonY + 20.f,
		(orientationButtonTop + sizeSliderBottom - 5.f) * 0.5f
	);
	auto* orientationLabel = CCLabelBMFont::create("Orientation", "bigFont.fnt");
	if (orientationLabel) {
		orientationLabel->setScale(0.4f);
		orientationLabel->setPosition({ size.width * 0.5f, orientationLabelY });
		m_contentArea->addChild(orientationLabel, 1);
	}
	const bool horizontal = state.orientation == geovision::VisualizerOrientation::Horizontal;
	const float orientationButtonWidth = std::min(100.f, size.width * 0.34f);
	auto addOrientationButton = [&](char const* text, geovision::VisualizerOrientation orientation, float x) {
		auto* sprite = ButtonSprite::create(text, orientationButtonWidth, 0, 0.36f, true, "bigFont.fnt", "GJ_button_01.png", 20.f);
		if (!sprite) return;
		auto* button = CCMenuItemSpriteExtra::create(
			sprite, this, menu_selector(GeovisionSettingsPopup::onOrientationSelected)
		);
		if (!button) return;
		button->setTag(static_cast<int>(orientation));
		button->setPosition({ x, orientationButtonY });
		menu->addChild(button);
	};
	addOrientationButton(
		horizontal ? "[ ] Vertical" : "[X] Vertical",
		geovision::VisualizerOrientation::Vertical,
		size.width * 0.29f
	);
	addOrientationButton(
		horizontal ? "[X] Horizontal" : "[ ] Horizontal",
		geovision::VisualizerOrientation::Horizontal,
		size.width * 0.72f
	);
}

void GeovisionSettingsPopup::onOrientationSelected(CCObject* sender) {
	auto* item = typeinfo_cast<CCNode*>(sender);
	if (!item) return;
	geovision::setVisualizerOrientation(
		static_cast<geovision::VisualizerOrientation>(item->getTag())
	);
	showPositionOptions();
}
