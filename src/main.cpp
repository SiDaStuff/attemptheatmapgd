#include "Heatmap.hpp"

using namespace geode::prelude;

$execute {
    listenForKeybindSettingPresses("toggle-heatmap", [](Keybind const&, bool down, bool repeat, double) {
        /*
            Keybind callbacks fire on both press and release, and repeat while held.
            Only accepting the first down event keeps a single tap from toggling
            the overlay several times.
        */
        if (down && !repeat && PlayLayer::get()) {
            heatmap::toggle();
        }
    });

    listenForKeybindSettingPresses("save-point", [](Keybind const&, bool down, bool repeat, double) {
        auto playLayer = PlayLayer::get();
        // This keybind is basically a manual marker for testing the heatmap
        // without having to actually die at the spot.
        if (down && !repeat && playLayer && playLayer->m_player1) {
            heatmap::addDeath(playLayer->m_level, playLayer->m_player1->getPosition());
            heatmap::refresh();
        }
    });

    listenForKeybindSettingPresses("clear-storage", [](Keybind const&, bool down, bool repeat, double) {
        // Clearing only touches the current level's key. Other levels keep their
        // points because levelKey() separates them.
        if (down && !repeat) {
            if (auto playLayer = PlayLayer::get()) {
                heatmap::clearDeaths(playLayer->m_level);
            }
        }
    });
}
