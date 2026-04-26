#include "Heatmap.hpp"

#include <algorithm>
#include <array>
#include <cerrno>
#include <cstdlib>
#include <cmath>
#include <string_view>

namespace heatmap {
    namespace {
        constexpr char const* kEnabledSaveKey = "heatmap-enabled";
        constexpr int kMaxStoredDeaths = 600;
        constexpr float kHeatRadius = 76.f;
        constexpr float kHeatRadiusSq = kHeatRadius * kHeatRadius;
        constexpr float kTau = 6.2831853f;
        constexpr int kCircleSegments = 48;

        constexpr std::array<std::pair<float, float>, 6> kGlowLayers = {{
            { 1.9f, .012f },
            { 1.55f, .02f },
            { 1.22f, .032f },
            { .92f, .055f },
            { .62f, .085f },
            { .34f, .13f },
        }};

        struct HeatPoint {
            CCPoint pos;
            float heat = 0.f;
        };

        std::array<CCPoint, kCircleSegments> const& circleVerts() {
            static auto verts = [] {
                std::array<CCPoint, kCircleSegments> out;
                for (auto i = 0; i < kCircleSegments; ++i) {
                    auto angle = (static_cast<float>(i) / static_cast<float>(kCircleSegments)) * kTau;
                    out[i].x = std::cos(angle);
                    out[i].y = std::sin(angle);
                }
                return out;
            }();

            return verts;
        }

        bool parseFloat(std::string_view text, float& out) {
            std::string copy(text);
            char* end = nullptr;
            errno = 0;

            auto value = std::strtof(copy.c_str(), &end);
            if (end != copy.c_str() + copy.size() || errno == ERANGE) {
                return false;
            }

            out = value;
            return true;
        }

        std::string levelKey(GJGameLevel* level) {
            if (!level) {
                return "level/unknown";
            }

            auto id = static_cast<int>(level->m_levelID.value());
            if (id > 0) {
                return fmt::format("level/{}/deaths", id);
            }

            return fmt::format("local/{}/deaths", level->m_levelName);
        }

        void saveDeaths(GJGameLevel* level, std::vector<DeathPoint> const& points) {
            std::string raw;
            raw.reserve(points.size() * 16);

            auto first = points.size() > kMaxStoredDeaths ? points.size() - kMaxStoredDeaths : 0;
            for (auto i = first; i < points.size(); ++i) {
                if (!raw.empty()) {
                    raw.push_back(';');
                }
                raw += fmt::format("{:.1f},{:.1f}", points[i].x, points[i].y);
            }

            Mod::get()->setSavedValue(levelKey(level), raw);
        }

        ccColor4F heatColor(float heat, float alpha) {
            heat = std::clamp(heat, 0.f, 1.f);

            if (heat < .45f) {
                auto t = heat / .45f;
                return {
                    .12f + (.88f * t),
                    .5f + (.42f * t),
                    1.f - (.88f * t),
                    alpha
                };
            }

            auto t = (heat - .45f) / .55f;
            return {
                1.f,
                .92f * (1.f - t),
                .1f * (1.f - t),
                alpha
            };
        }

        void fillDisc(CCDrawNode* node, CCPoint const& pos, float radius, ccColor4F const& color) {
            std::array<CCPoint, kCircleSegments> verts;
            auto const& unit = circleVerts();

            for (auto i = 0; i < kCircleSegments; ++i) {
                verts[i].x = pos.x + (unit[i].x * radius);
                verts[i].y = pos.y + (unit[i].y * radius);
            }

            node->drawPolygon(verts.data(), verts.size(), color, 0.f, { 0.f, 0.f, 0.f, 0.f });
        }

        std::vector<HeatPoint> buildHeat(std::vector<DeathPoint> const& deaths) {
            std::vector<HeatPoint> out;
            out.reserve(deaths.size());
            auto peak = 1.f;

            for (auto const& death : deaths) {
                HeatPoint point = { { death.x, death.y }, 0.f };

                for (auto const& other : deaths) {
                    auto dx = death.x - other.x;
                    auto dy = death.y - other.y;
                    auto distSq = (dx * dx) + (dy * dy);

                    if (distSq <= kHeatRadiusSq) {
                        auto falloff = 1.f - (distSq / kHeatRadiusSq);
                        point.heat += falloff * falloff;
                    }
                }

                peak = std::max(peak, point.heat);
                out.push_back(point);
            }

            for (auto& point : out) {
                point.heat /= peak;
            }

            std::ranges::sort(out, [](HeatPoint const& a, HeatPoint const& b) {
                return a.heat < b.heat;
            });

            return out;
        }

        Overlay* overlayFor(PlayLayer* playLayer) {
            if (!playLayer || !playLayer->m_objectLayer) {
                return nullptr;
            }

            return typeinfo_cast<Overlay*>(playLayer->m_objectLayer->getChildByID(kOverlayNodeID));
        }
    }

    bool isEnabled() {
        return Mod::get()->getSavedValue<bool>(kEnabledSaveKey, true);
    }

    void setEnabled(bool enabled) {
        Mod::get()->setSavedValue(kEnabledSaveKey, enabled);
    }

    void toggle() {
        setEnabled(!isEnabled());
        refresh();
    }

    std::vector<DeathPoint> loadDeaths(GJGameLevel* level) {
        auto raw = Mod::get()->getSavedValue<std::string>(levelKey(level), "");
        std::vector<DeathPoint> points;
        points.reserve(std::min<size_t>(raw.size() / 8, kMaxStoredDeaths));

        size_t start = 0;
        while (start < raw.size()) {
            auto end = raw.find(';', start);
            auto item = std::string_view(raw).substr(
                start,
                end == std::string::npos ? std::string::npos : end - start
            );
            auto comma = item.find(',');

            if (comma != std::string_view::npos) {
                DeathPoint point;
                auto xs = item.substr(0, comma);
                auto ys = item.substr(comma + 1);

                if (parseFloat(xs, point.x) && parseFloat(ys, point.y)) {
                    points.push_back(point);
                }
            }

            if (end == std::string::npos) {
                break;
            }
            start = end + 1;
        }

        return points;
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
        this->setID(kOverlayNodeID);
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
        if (deaths.empty()) {
            return;
        }

        for (auto const& point : buildHeat(deaths)) {
            auto heat = std::pow(point.heat, .7f);
            auto radius = 13.f + (heat * 18.f);

            for (auto [scale, alpha] : kGlowLayers) {
                fillDisc(this, point.pos, radius * scale, heatColor(heat, alpha));
            }
        }
    }
}
