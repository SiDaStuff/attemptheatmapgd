#include "Heatmap.hpp"

#include <Geode/modify/PauseLayer.hpp>

using namespace geode::prelude;

class $modify(HeatmapPauseLayer, PauseLayer) {
    struct Fields {
        ButtonSprite* m_toggleSprite = nullptr;
    };

    char const* toggleText() const {
        return heatmap::isEnabled() ? "Heatmap: On" : "Heatmap: Off";
    }

    void customSetup() {
        PauseLayer::customSetup();

        auto winSize = CCDirector::sharedDirector()->getWinSize();

        auto sprite = ButtonSprite::create(this->toggleText(), 110, true, "bigFont.fnt", "GJ_button_01.png", 28.f, .55f);
        auto button = CCMenuItemSpriteExtra::create(
            sprite,
            this,
            menu_selector(HeatmapPauseLayer::onHeatmapToggle)
        );

        auto menu = CCMenu::create();
        menu->setID("attempt-heatmap-menu"_spr);

        /*
            This sits in the lower-right pause corner, outside the normal center
            controls. Anchoring from winSize keeps it stable across aspect ratios.
        */
        menu->setPosition({ winSize.width - 78.f, 31.f });
        menu->addChild(button);
        this->addChild(menu, 20);

        // Keep the ButtonSprite pointer so only the label has to change after a
        // toggle. Rebuilding the pause menu for one word would be overkill.
        m_fields->m_toggleSprite = sprite;
    }

    void onHeatmapToggle(CCObject*) {
        heatmap::toggle();

        if (m_fields->m_toggleSprite) {
            m_fields->m_toggleSprite->setString(this->toggleText());
        }
    }
};
