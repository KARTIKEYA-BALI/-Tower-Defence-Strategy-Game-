#include "spell_book.h"
#include <algorithm>
#include <cmath>

namespace {

const float PI_F = 3.14159265358979f;

}

SpellBook::SpellBook() {
    types.push_back({SpellKind::Freeze, "freeze", "Freeze", 2, 14.0f, {{30, 2.0f}, {50, 3.5f}, {75, 5.0f}},
                     {130, 200, 255, 255}, "Every enemy in the blast is frozen solid. Bosses thaw twice as fast.",
                     "Stops enemies"});
    types.push_back({SpellKind::Mud, "mud", "Mud", 2, 8.0f, {{15, 5.0f}, {25, 8.0f}, {40, 12.0f}},
                     {150, 110, 64, 255}, "A cheap patch of sticky mud. Enemies wade through it at under half speed.",
                     "45% speed"});
    types.push_back({SpellKind::Fire, "fire", "Fire", 1, 10.0f, {{25, 4.0f}, {40, 6.0f}, {60, 9.0f}},
                     {255, 120, 40, 255}, "Sets the ground alight. Anything that walks through it gets burned.",
                     "14 dmg / s"});
    reset();
}

const SpellType& SpellBook::type(int kind) const {
    if (!validKind(kind)) return types[0];
    return types[kind];
}

std::vector<int> SpellBook::all() const {
    std::vector<int> out;
    for (int i = 0; i < count(); i++) out.push_back(i);
    return out;
}

bool SpellBook::ready(int kind) const {
    return validKind(kind) && cooldowns[kind] <= 0.0f;
}

float SpellBook::cooldownLeft(int kind) const {
    if (!validKind(kind)) return 0.0f;
    return std::max(0.0f, cooldowns[kind]);
}

float SpellBook::cooldownFraction(int kind) const {
    if (!validKind(kind) || types[kind].cooldown <= 0.0f) return 0.0f;
    return std::clamp(cooldowns[kind] / types[kind].cooldown, 0.0f, 1.0f);
}

void SpellBook::startCooldown(int kind) {
    if (validKind(kind)) cooldowns[kind] = types[kind].cooldown;
}

void SpellBook::update(float dt) {
    for (float& c : cooldowns) c = std::max(0.0f, c - dt);
}

void SpellBook::reset() {
    for (float& c : cooldowns) c = 0.0f;
}

const char* SpellBook::tierName(int tier) {
    static const char* names[SPELL_TIERS] = {"I", "II", "III"};
    if (!validTier(tier)) return "";
    return names[tier];
}

void SpellBook::drawIcon(Gfx& g, int kind, float cx, float cy, float r, float time) {
    if (kind == static_cast<int>(SpellKind::Freeze)) {
        g.fillCircleGradient(cx, cy, r, rgba(70, 120, 180), rgba(24, 40, 70));
        for (int i = 0; i < 3; i++) {
            float a = i * PI_F / 3.0f;
            float dx = std::cos(a) * r * 0.72f;
            float dy = std::sin(a) * r * 0.72f;
            g.line(cx - dx, cy - dy, cx + dx, cy + dy, std::max(1.4f, r * 0.12f), rgba(220, 240, 255));
            for (int s = -1; s <= 1; s += 2) {
                float bx = cx + dx * 0.55f * s;
                float by = cy + dy * 0.55f * s;
                float px = -std::sin(a) * r * 0.18f;
                float py = std::cos(a) * r * 0.18f;
                g.line(bx, by, bx + dx * 0.25f * s + px, by + dy * 0.25f * s + py, 1.1f, rgba(220, 240, 255));
                g.line(bx, by, bx + dx * 0.25f * s - px, by + dy * 0.25f * s - py, 1.1f, rgba(220, 240, 255));
            }
        }
        g.fillCircle(cx, cy, r * 0.14f, rgba(255, 255, 255));
    } else if (kind == static_cast<int>(SpellKind::Mud)) {
        g.fillCircleGradient(cx, cy, r, rgba(96, 72, 44), rgba(44, 32, 20));
        g.fillEllipse(cx, cy + r * 0.2f, r * 0.7f, r * 0.42f, rgba(120, 86, 50));
        g.fillEllipse(cx - r * 0.1f, cy + r * 0.12f, r * 0.45f, r * 0.24f, rgba(150, 110, 64));
        float bob = std::sin(time * 3.0f) * r * 0.05f;
        g.fillCircle(cx + r * 0.32f, cy - r * 0.18f + bob, r * 0.14f, rgba(160, 120, 74));
        g.fillCircle(cx - r * 0.34f, cy - r * 0.05f - bob, r * 0.1f, rgba(160, 120, 74));
        g.fillCircle(cx + r * 0.05f, cy + r * 0.05f, r * 0.08f, rgba(210, 180, 140, 200));
    } else {
        g.fillCircleGradient(cx, cy, r, rgba(120, 40, 20), rgba(50, 14, 8));
        float flick = 0.5f + 0.5f * std::sin(time * 9.0f);
        g.glow(cx, cy, r * 0.9f, r * 0.9f, rgba(255, 120, 30, 90));
        std::vector<SDL_FPoint> outer = {{cx - r * 0.5f, cy + r * 0.5f}, {cx - r * 0.55f, cy - r * 0.05f},
                                         {cx - r * 0.2f, cy - r * 0.35f}, {cx, cy - r * (0.75f + 0.08f * flick)},
                                         {cx + r * 0.25f, cy - r * 0.3f}, {cx + r * 0.55f, cy - r * 0.05f},
                                         {cx + r * 0.5f, cy + r * 0.5f}};
        g.fillPolygon(outer, rgba(240, 96, 30));
        std::vector<SDL_FPoint> inner = {{cx - r * 0.28f, cy + r * 0.48f}, {cx - r * 0.25f, cy + r * 0.05f},
                                         {cx, cy - r * (0.35f + 0.08f * flick)}, {cx + r * 0.25f, cy + r * 0.05f},
                                         {cx + r * 0.28f, cy + r * 0.48f}};
        g.fillPolygon(inner, rgba(255, 210, 90));
    }
    g.strokeCircle(cx, cy, r, 1.2f, rgba(0, 0, 0, 120));
}
