#pragma once

#include <Geode/Geode.hpp>

#include <vector>

using namespace geode::prelude;

namespace heatmap {
    struct DeathPoint {
        float x;
        float y;
    };

    bool isEnabled();
    void toggle();

    void addDeath(GJGameLevel* level, CCPoint const& pos);
    void clearDeaths(GJGameLevel* level);
    void refresh();

    class Overlay final : public CCDrawNode {
    public:
        static Overlay* create(GJGameLevel* level);

        bool init(GJGameLevel* level);
        void redraw();

    private:
        GJGameLevel* m_level = nullptr;
    };
}
