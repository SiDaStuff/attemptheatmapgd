#include "Heatmap.hpp"

using namespace geode::prelude;

$execute {
    listenForKeybindSettingPresses(heatmap::kToggleKeybind, [](Keybind const&, bool down, bool repeat, double) {
        if (down && !repeat && PlayLayer::get()) {
            heatmap::toggle();
        }
    });

    listenForKeybindSettingPresses(heatmap::kSavePointKeybind, [](Keybind const&, bool down, bool repeat, double) {
        auto playLayer = PlayLayer::get();
        if (down && !repeat && playLayer && playLayer->m_player1) {
            heatmap::addDeath(playLayer->m_level, playLayer->m_player1->getPosition());
            heatmap::refresh();
        }
    });

    listenForKeybindSettingPresses(heatmap::kClearStorageKeybind, [](Keybind const&, bool down, bool repeat, double) {
        if (down && !repeat) {
            if (auto playLayer = PlayLayer::get()) {
                heatmap::clearDeaths(playLayer->m_level);
            }
        }
    });
}
