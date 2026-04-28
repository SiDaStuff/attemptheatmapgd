#include "Heatmap.hpp"

#include <cmath>
#include <sstream>
#include <string>

namespace heatmap {
    static constexpr char const* enabledSaveKey = "heatmap-enabled";
    static constexpr char const* overlayNodeID = "attempt-heatmap-overlay";
    static constexpr int maxSavedDeaths = 600;

    /*
        Each level needs its own save key so deaths from one level do not appear
        on another level. Online levels have a stable numeric ID. Local levels
        may not, so their name is used as the fallback.
    */
    static std::string levelKey(GJGameLevel* level) {
        if (!level) {
            return "level/unknown/deaths";
        }

        auto id = static_cast<int>(level->m_levelID.value());
        if (id > 0) {
            return fmt::format("level/{}/deaths", id);
        }

        return fmt::format("local/{}/deaths", level->m_levelName);
    }

    /*
        Geode saved values are easiest to store as one string here. The format is
        "x,y;x,y;x,y". It keeps the mod small and avoids adding a file format or
        extra dependency just for a list of points.
    */
    static std::vector<DeathPoint> loadDeaths(GJGameLevel* level) {
        auto raw = Mod::get()->getSavedValue<std::string>(levelKey(level), "");
        std::vector<DeathPoint> points;

        std::stringstream list(raw);
        std::string item;
        while (std::getline(list, item, ';')) {
            std::stringstream pair(item);
            DeathPoint point {};
            char comma = 0;

            if (pair >> point.x >> comma >> point.y && comma == ',') {
                points.push_back(point);
            }
        }

        return points;
    }

    static void saveDeaths(GJGameLevel* level, std::vector<DeathPoint> const& points) {
        std::string raw;

        /*
            The cap stops the saved value from growing forever on levels that get
            played a lot. Keeping the newest points matters most because they
            represent the player's current practice session.
        */
        auto first = points.size() > maxSavedDeaths ? points.size() - maxSavedDeaths : 0;
        for (auto i = first; i < points.size(); ++i) {
            if (!raw.empty()) {
                raw.push_back(';');
            }

            raw += fmt::format("{:.1f},{:.1f}", points[i].x, points[i].y);
        }

        Mod::get()->setSavedValue(levelKey(level), raw);
    }

    static Overlay* overlayFor(PlayLayer* playLayer) {
        if (!playLayer || !playLayer->m_objectLayer) {
            return nullptr;
        }

        return typeinfo_cast<Overlay*>(playLayer->m_objectLayer->getChildByID(overlayNodeID));
    }

    static void fillCircle(CCDrawNode* node, CCPoint const& center, float radius, ccColor4F const& color) {
        /*
            CCDrawNode draws polygons, not real circles. 28 points is enough to
            look round at this size while staying cheap when many deaths exist. Yes, I did use AI to find these values.
        */
        constexpr int circleSegments = 28;
        constexpr float fullTurn = 6.2831853f;
        CCPoint verts[circleSegments];

        for (auto i = 0; i < circleSegments; ++i) {
            auto angle = (static_cast<float>(i) / static_cast<float>(circleSegments)) * fullTurn;
            verts[i].x = center.x + (std::cos(angle) * radius);
            verts[i].y = center.y + (std::sin(angle) * radius);
        }

        node->drawPolygon(verts, circleSegments, color, 0.f, { 0.f, 0.f, 0.f, 0.f });
    }

    static void drawDeath(CCDrawNode* node, DeathPoint const& death) {
        auto pos = CCPoint { death.x, death.y };

        /*
            The overlay uses additive blending, so these low-alpha circles get
            brighter when several deaths overlap. That creates the heatmap effect
            without a separate density algorithm. Yes, I did use AI to find these values.
        */
        fillCircle(node, pos, 26.f, { 1.f, .55f, .03f, .10f });
        fillCircle(node, pos, 8.f, { 1.f, .1f, .02f, .25f });
    }

    bool isEnabled() {
        return Mod::get()->getSavedValue<bool>(enabledSaveKey, true);
    }

    void toggle() {
        Mod::get()->setSavedValue(enabledSaveKey, !isEnabled());
        refresh();
    }

    void addDeath(GJGameLevel* level, CCPoint const& pos) {
        auto points = loadDeaths(level);
        points.push_back({ pos.x, pos.y });
        saveDeaths(level, points);
    }

    void clearDeaths(GJGameLevel* level) {
        Mod::get()->setSavedValue(levelKey(level), std::string());
        refresh();
    }

    void refresh() {
        if (auto playLayer = PlayLayer::get()) {
            if (auto overlay = overlayFor(playLayer)) {
                overlay->redraw();
            }
        }
    }

    Overlay* Overlay::create(GJGameLevel* level) {
        /*
            Cocos2d objects use create/init/autorelease instead of plain stack
            objects. This matches how Geode expects CCNode subclasses to be made.
        */
        auto ret = new Overlay();
        if (ret && ret->init(level)) {
            ret->autorelease();
            return ret;
        }

        delete ret;
        return nullptr;
    }

    bool Overlay::init(GJGameLevel* level) {
        if (!CCDrawNode::init()) {
            return false;
        }

        m_level = level;
        this->setID(overlayNodeID);
        this->setBlendFunc({ GL_SRC_ALPHA, GL_ONE });
        this->redraw();
        return true;
    }

    void Overlay::redraw() {
        this->clear();

        if (!isEnabled()) {
            this->setVisible(false);
            return;
        }

        this->setVisible(true);
        auto deaths = loadDeaths(m_level);
        for (auto const& death : deaths) {
            drawDeath(this, death);
        }
    }
}
