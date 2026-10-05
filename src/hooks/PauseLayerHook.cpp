#include <Geode/Geode.hpp>
#include <Geode/modify/PauseLayer.hpp>

#include "../ui/GeovisionSettingsPopup.hpp"

using namespace geode::prelude;

class $modify(GeovisionPauseLayer, PauseLayer) {
	struct Fields {
		bool buttonAdded = false;
	};

	void customSetup() {
		PauseLayer::customSetup();
		if (m_fields->buttonAdded) return;

		auto* icon = CCSprite::createWithSpriteFrameName("GJ_playMusicBtn_001.png");
		if (!icon) {
			log::warn("Geovision: couldn't create pause button music icon");
			return;
		}
		icon->setScale(0.75f);

		auto* button = CCMenuItemSpriteExtra::create(
			icon,
			this,
			menu_selector(GeovisionPauseLayer::onGeovisionSettings)
		);
		if (!button) return;
		button->setID("geovision-settings-button"_spr);
		button->setSizeMult(1.25f);

		auto* menu = CCMenu::create(button, nullptr);
		if (!menu) return;
		menu->setID("geovision-settings-menu"_spr);
		menu->setPosition(CCPointZero);
		const auto winSize = CCDirector::sharedDirector()->getWinSize();
		button->setPosition({ winSize.width - 34.f, winSize.height * 0.16f });
		this->addChild(menu, 1000);
		m_fields->buttonAdded = true;
		log::info("Geovision: added settings button to PauseLayer");
	}

	void onGeovisionSettings(CCObject*) {
		if (auto* popup = GeovisionSettingsPopup::create()) {
			popup->show();
		}
	}
};
