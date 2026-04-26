#pragma once

#include <Geode/Geode.hpp>

#include <string>
#include <vector>

using namespace geode::prelude;

namespace heatmap {
    inline constexpr char const* kToggleKeybind = "toggle-heatmap";
    inline constexpr char const* kSavePointKeybind = "save-point";
    inline constexpr char const* kClearStorageKeybind = "clear-storage";
    inline constexpr char const* kOverlayNodeID = "attempt-heatmap-overlay";

    struct DeathPoint {
        float x = 0.f;
        float y = 0.f;
    };

    bool isEnabled();
    void setEnabled(bool enabled);
    void toggle();

    std::vector<DeathPoint> loadDeaths(GJGameLevel* level);
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
