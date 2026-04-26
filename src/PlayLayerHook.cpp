#include "Heatmap.hpp"

#include <Geode/modify/PlayLayer.hpp>

using namespace geode::prelude;

class $modify(HeatmapPlayLayer, PlayLayer) {
    static constexpr auto kMinLiveFrames = 3;
    static constexpr auto kMinTravelSq = 12.f * 12.f;

    struct Fields {
        CCPoint m_spawnPos = CCPointZero;
        CCPoint m_lastGoodPos = CCPointZero;
        int m_liveFrames = 0;
        bool m_readyToRecord = false;
        bool m_hasGoodPos = false;
    };

    void resetAttemptTracker() {
        m_fields->m_liveFrames = 0;
        m_fields->m_readyToRecord = false;
        m_fields->m_hasGoodPos = false;

        if (m_player1) {
            m_fields->m_spawnPos = m_player1->m_position;
            m_fields->m_lastGoodPos = m_player1->m_position;
        }
    }

    void updateAttemptTracker() {
        if (!m_player1 || m_player1->m_isDead || m_playerDied) {
            return;
        }

        auto pos = m_player1->m_position;
        auto dx = pos.x - m_fields->m_spawnPos.x;
        auto dy = pos.y - m_fields->m_spawnPos.y;
        auto travelSq = (dx * dx) + (dy * dy);

        m_fields->m_liveFrames += 1;

        if (m_fields->m_liveFrames >= kMinLiveFrames && travelSq > kMinTravelSq) {
            m_fields->m_readyToRecord = true;
        }

        if (m_fields->m_readyToRecord) {
            m_fields->m_lastGoodPos = pos;
            m_fields->m_hasGoodPos = true;
        }
    }

    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        if (!PlayLayer::init(level, useReplay, dontCreateObjects)) {
            return false;
        }

        this->resetAttemptTracker();

        if (m_objectLayer) {
            if (auto overlay = heatmap::Overlay::create(level)) {
                m_objectLayer->addChild(overlay, 9999);
            }
        }

        return true;
    }

    void postUpdate(float dt) {
        this->updateAttemptTracker();
        PlayLayer::postUpdate(dt);
    }

    void destroyPlayer(PlayerObject* player, GameObject* object) {
        this->updateAttemptTracker();

        if (m_fields->m_readyToRecord && m_fields->m_hasGoodPos) {
            heatmap::addDeath(m_level, m_fields->m_lastGoodPos);
        }

        PlayLayer::destroyPlayer(player, object);
        this->resetAttemptTracker();
        heatmap::refresh();
    }

    void resetLevel() {
        PlayLayer::resetLevel();
        this->resetAttemptTracker();
        heatmap::refresh();
    }
};
