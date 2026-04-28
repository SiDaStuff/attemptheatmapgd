#include "Heatmap.hpp"

#include <Geode/modify/PlayLayer.hpp>

using namespace geode::prelude;

class $modify(HeatmapPlayLayer, PlayLayer) {
    struct Fields {
        float m_startX = 0.f;
        bool m_movedFromStart = false;
    };

    void resetHeatmapAttempt() {
        m_fields->m_startX = m_player1 ? m_player1->getPositionX() : 0.f;
        m_fields->m_movedFromStart = false;
    }

    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        if (!PlayLayer::init(level, useReplay, dontCreateObjects)) {
            return false;
        }

        this->resetHeatmapAttempt();

        /*
            The overlay is attached to the object layer so it scrolls with the
            level. A high z value keeps it above level objects without needing a
            separate camera or screen-space conversion.
        */
        if (m_objectLayer) {
            if (auto overlay = heatmap::Overlay::create(level)) {
                m_objectLayer->addChild(overlay, 9999);
            }
        }

        return true;
    }

    void postUpdate(float dt) {
        PlayLayer::postUpdate(dt);

        /*
            Geometry Dash can call death/reset code while the player is still at
            the level spawn. Waiting until the player has moved right keeps those
            reset deaths from being saved as a heat spot at the beginning.
        */
        if (m_player1 && m_player1->getPositionX() > m_fields->m_startX + 20.f) {
            m_fields->m_movedFromStart = true;
        }
    }

    void destroyPlayer(PlayerObject* player, GameObject* object) {
        /*
            The death position is saved before calling the original function
            because the original death code can change player state. m_playerDied
            prevents dual mode from recording the same attempt twice.
        */
        if (player && !m_playerDied && m_fields->m_movedFromStart) {
            heatmap::addDeath(m_level, player->getPosition());
        }

        PlayLayer::destroyPlayer(player, object);
        heatmap::refresh();
    }

    void resetLevel() {
        PlayLayer::resetLevel();
        this->resetHeatmapAttempt();
        heatmap::refresh();
    }
};
