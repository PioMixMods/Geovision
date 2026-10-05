#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>

#include "../audio/FmodAudioAnalyzer.hpp"
#include "../config/MatchBackgroundColor.hpp"
#include "../config/Optimization.hpp"
#include "../config/VisualizerPosition.hpp"
#include "../config/VisualizerColor.hpp"
#include "../visualizers/BarsVisualizer.hpp"
#include "../visualizers/WaveVisualizer.hpp"
#include "../visualizers/RadialVisualizer.hpp"

#include <memory>

using namespace geode::prelude;

class $modify(GeovisionPlayLayer, PlayLayer) {
	struct Fields {
		std::unique_ptr<geovision::FmodAudioAnalyzer> analyzer;
		std::unique_ptr<geovision::Visualizer> visualizer;
		CCNode* visualizerContainer = nullptr;
		bool backgroundColorOverrideActive = false;
		bool hasLastBackgroundColor = false;
		cocos2d::ccColor3B lastBackgroundColor = { 0, 0, 0 };
		geovision::VisualizerType activeVisualizerType = geovision::VisualizerType::Bars;
		bool loggedFirstUpdate = false;
		bool paused = false;
	};

	void onEnterTransitionDidFinish() {
		PlayLayer::onEnterTransitionDidFinish();
		startGeovision();
	}

	void postUpdate(float dt) {
		PlayLayer::postUpdate(dt);
		syncVisualizer();
		updateVisualizerBackgroundColor();

		if (m_fields->paused) return;
		if (!m_fields->analyzer || !m_fields->visualizer) return;
		if (!m_fields->loggedFirstUpdate) {
			log::info("Geovision: PlayLayer::postUpdate is updating the visualizer");
			m_fields->loggedFirstUpdate = true;
		}
		const auto frame = m_fields->analyzer->update(dt);
		m_fields->visualizer->update(dt, frame);
	}

	void pauseGame(bool unfocused) {
		m_fields->paused = true;
		if (m_fields->analyzer) m_fields->analyzer->reset();
		if (m_fields->visualizerContainer) {
			m_fields->visualizerContainer->setVisible(false);
		}
		PlayLayer::pauseGame(unfocused);
	}

	void resume() {
		PlayLayer::resume();
		m_fields->paused = false;
		startGeovision();
		if (m_fields->visualizerContainer) {
			m_fields->visualizerContainer->setVisible(true);
		}
	}

	void onExit() {
		stopGeovision();
		PlayLayer::onExit();
	}

	void startGeovision() {
		log::info("Geovision: initializing PlayLayer visualizer");
		if (!m_fields->analyzer) {
			m_fields->analyzer = std::make_unique<geovision::FmodAudioAnalyzer>();
		}
		if (!m_fields->visualizerContainer) {
			const auto winSize = CCDirector::sharedDirector()->getWinSize();
			auto* overlayParent = this->getParent();
			if (!overlayParent) overlayParent = this;
			auto* container = CCNode::create();
			if (container) {
				container->setContentSize(winSize);
				container->setAnchorPoint({ 0.5f, 0.5f });
				container->setPosition({ winSize.width * 0.5f, winSize.height * 0.5f });
				overlayParent->addChild(container, 10);
				m_fields->visualizerContainer = container;
				geovision::attachVisualizerPositionContainer(container);
			}
		}
		syncVisualizer();
	}

	void updateVisualizerBackgroundColor() {
		const bool enabled = geovision::matchBackgroundColorEnabled();
		if (!enabled) {
			if (m_fields->backgroundColorOverrideActive) {
				geovision::setVisualizerBackgroundColorOverride(false);
				m_fields->backgroundColorOverrideActive = false;
			}
			m_fields->hasLastBackgroundColor = false;
			return;
		}

		if (!m_background) return;
		const auto color = m_background->getColor();
		const bool changed = !m_fields->hasLastBackgroundColor ||
			color.r != m_fields->lastBackgroundColor.r ||
			color.g != m_fields->lastBackgroundColor.g ||
			color.b != m_fields->lastBackgroundColor.b;
		if (changed || !m_fields->backgroundColorOverrideActive) {
			const bool normalRedrawThisFrame = !m_fields->paused &&
				m_fields->analyzer && m_fields->visualizer;
			const bool notifyVisualizer = !geovision::avoidDuplicateRedrawEnabled() || !normalRedrawThisFrame;
			geovision::setVisualizerBackgroundColorOverride(true, color, notifyVisualizer);
			m_fields->lastBackgroundColor = color;
			m_fields->hasLastBackgroundColor = true;
			m_fields->backgroundColorOverrideActive = true;
		}
	}

	void syncVisualizer() {
		const auto selectedType = geovision::g_selectedVisualizerType;
		if (m_fields->visualizer && m_fields->activeVisualizerType == selectedType) return;
		if (m_fields->visualizer) {
			m_fields->visualizer->cleanup();
			m_fields->visualizer.reset();
		}

		std::unique_ptr<geovision::Visualizer> visualizer;
		if (selectedType == geovision::VisualizerType::Wave) {
			visualizer = std::make_unique<geovision::WaveVisualizer>();
		} else if (selectedType == geovision::VisualizerType::Radial) {
			visualizer = std::make_unique<geovision::RadialVisualizer>();
		} else {
			visualizer = std::make_unique<geovision::BarsVisualizer>();
		}
		if (!m_fields->visualizerContainer) return;
		if (visualizer->init(m_fields->visualizerContainer)) {
			geovision::setVisualizerPositionBounds(
				visualizer->getBounds(CCDirector::sharedDirector()->getWinSize())
			);
			m_fields->activeVisualizerType = selectedType;
			m_fields->visualizer = std::move(visualizer);
			const char* visualizerName = selectedType == geovision::VisualizerType::Wave ? "Wave" :
				selectedType == geovision::VisualizerType::Radial ? "Radial" : "Bars";
			log::info("Geovision: {} visualizer initialized", visualizerName);
		} else {
			const char* visualizerName = selectedType == geovision::VisualizerType::Wave ? "Wave" :
				selectedType == geovision::VisualizerType::Radial ? "Radial" : "Bars";
			log::warn("Geovision: {} visualizer initialization failed", visualizerName);
		}
	}

	void stopGeovision() {
		if (m_fields->visualizer) {
			m_fields->visualizer->cleanup();
			m_fields->visualizer.reset();
		}
		m_fields->activeVisualizerType = geovision::g_selectedVisualizerType;
		if (m_fields->analyzer) {
			m_fields->analyzer->reset();
			m_fields->analyzer.reset();
		}
		if (m_fields->visualizerContainer) {
			geovision::detachVisualizerPositionContainer();
			m_fields->visualizerContainer->removeFromParentAndCleanup(true);
			m_fields->visualizerContainer = nullptr;
		}
		geovision::setVisualizerBackgroundColorOverride(false);
		m_fields->backgroundColorOverrideActive = false;
		m_fields->hasLastBackgroundColor = false;
		m_fields->paused = false;
		m_fields->loggedFirstUpdate = false;
	}
};
