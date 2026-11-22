#include "theme.h"
#include "engine_core.h"
#include <cmath>

namespace {

const Theme THEMES[THEME_COUNT] = {
    {"Meadow", {40, 64, 40, 255}, {60, 92, 52, 255}, {88, 120, 64, 255}, {96, 108, 58, 255}, {110, 84, 56, 255},
     {156, 124, 82, 255}, {78, 82, 74, 255}, {{222, 210, 150, 255}, {204, 160, 184, 255}, {214, 216, 210, 255}}, 5,
     {74, 78, 88, 255}, {126, 130, 138, 255}, {176, 178, 182, 255}, {84, 116, 62, 170}, Feature::Bush, Ambient::None,
     false, {0, 0, 0, 0}, {4, 8, 6, 120}, {64, 96, 54, 255}, {58, 89, 50, 255}, {150, 118, 78, 255},
     {40, 90, 40, 255}, {112, 86, 58, 255}, {52, 38, 28, 255}, {26, 44, 60, 255}, {10, 16, 24, 255}},
    {"Snow", {170, 184, 202, 255}, {206, 216, 228, 255}, {238, 242, 248, 255}, {186, 204, 224, 255}, {116, 126, 142, 255},
     {156, 166, 182, 255}, {130, 140, 156, 255}, {{255, 255, 255, 255}, {190, 216, 240, 255}, {150, 168, 190, 255}}, 3,
     {66, 78, 98, 255}, {116, 130, 150, 255}, {196, 210, 226, 255}, {244, 248, 252, 230}, Feature::Pine, Ambient::Snow,
     false, {0, 0, 0, 0}, {10, 18, 36, 110}, {214, 222, 232, 255}, {200, 210, 224, 255}, {140, 150, 168, 255},
     {40, 78, 64, 255}, {150, 164, 184, 255}, {64, 72, 90, 255}, {44, 60, 86, 255}, {14, 20, 34, 255}},
    {"Desert", {184, 146, 92, 255}, {212, 176, 116, 255}, {232, 204, 146, 255}, {200, 146, 88, 255}, {146, 108, 68, 255},
     {176, 136, 88, 255}, {156, 126, 92, 255}, {{126, 96, 66, 255}, {242, 222, 172, 255}, {170, 118, 78, 255}}, 3,
     {122, 74, 48, 255}, {182, 116, 74, 255}, {226, 168, 118, 255}, {0, 0, 0, 0}, Feature::Cactus, Ambient::Dust, false,
     {0, 0, 0, 0}, {44, 22, 6, 120}, {220, 186, 128, 255}, {210, 174, 116, 255}, {160, 120, 76, 255},
     {70, 126, 70, 255}, {170, 116, 70, 255}, {86, 52, 32, 255}, {70, 50, 40, 255}, {22, 14, 12, 255}},
    {"Autumn", {84, 70, 36, 255}, {124, 100, 46, 255}, {158, 126, 58, 255}, {150, 80, 38, 255}, {100, 72, 46, 255},
     {138, 102, 66, 255}, {88, 82, 68, 255}, {{214, 100, 40, 255}, {232, 172, 52, 255}, {168, 48, 30, 255}}, 2,
     {72, 74, 72, 255}, {126, 124, 116, 255}, {176, 172, 162, 255}, {92, 118, 54, 190}, Feature::AutumnTree,
     Ambient::Leaves, false, {0, 0, 0, 0}, {22, 10, 2, 120}, {132, 106, 50, 255}, {122, 98, 46, 255},
     {150, 110, 72, 255}, {196, 98, 36, 255}, {104, 76, 46, 255}, {44, 30, 20, 255}, {44, 34, 30, 255},
     {14, 10, 10, 255}},
    {"Volcanic", {34, 30, 32, 255}, {50, 44, 44, 255}, {66, 58, 56, 255}, {64, 40, 32, 255}, {84, 74, 68, 255},
     {112, 98, 88, 255}, {38, 34, 34, 255}, {{120, 110, 104, 255}, {90, 50, 36, 255}, {150, 70, 40, 255}}, 6,
     {16, 14, 18, 255}, {40, 36, 44, 255}, {80, 74, 86, 255}, {0, 0, 0, 0}, Feature::LavaPool, Ambient::Embers, false,
     {255, 106, 30, 255}, {34, 6, 0, 120}, {58, 50, 50, 255}, {50, 44, 44, 255}, {112, 98, 88, 255},
     {240, 96, 30, 255}, {70, 58, 56, 255}, {26, 20, 22, 255}, {58, 22, 14, 255}, {14, 6, 6, 255}},
};

float h01(unsigned seed, int k) {
    unsigned h = seed * 2654435761u + static_cast<unsigned>(k) * 40503u;
    h = (h ^ (h >> 13)) * 1274126177u;
    h ^= h >> 16;
    return static_cast<float>(h & 0xFFFFu) / 65536.0f;
}

void drawBush(Gfx& g, float cx, float cy, unsigned seed) {
    g.glow(cx + 2.0f, cy + 12.0f, 20.0f, 7.0f, rgba(0, 0, 0, 120));
    for (int i = 0; i < 4; i++) {
        float ox = (h01(seed, i) - 0.5f) * 22.0f;
        float oy = (h01(seed, i + 9) - 0.5f) * 12.0f;
        float r = 9.0f + h01(seed, i + 3) * 5.0f;
        g.fillCircle(cx + ox, cy + oy, r, rgba(36, 72, 34));
        g.fillCircle(cx + ox - 2.0f, cy + oy - 2.0f, r * 0.75f, rgba(62, 110, 50));
    }
}

void drawPine(Gfx& g, float cx, float cy, unsigned seed) {
    float s = 0.85f + h01(seed, 1) * 0.3f;
    float ox = (h01(seed, 2) - 0.5f) * 8.0f;
    cx += ox;
    float base = cy + 18.0f * s;
    g.glow(cx + 4.0f, base, 16.0f * s, 6.0f * s, rgba(20, 30, 60, 120));
    g.fillRect(cx - 2.5f * s, base - 8.0f * s, 5.0f * s, 8.0f * s, rgba(78, 52, 36));
    const Color dark = rgba(30, 64, 54);
    const Color mid = rgba(42, 86, 68);
    for (int tier = 0; tier < 3; tier++) {
        float w = (17.0f - tier * 4.0f) * s;
        float bottom = base - (6.0f + tier * 11.0f) * s;
        float top = bottom - 17.0f * s;
        g.fillTriangle({cx - w, bottom}, {cx + w, bottom}, {cx, top}, dark);
        g.fillTriangle({cx - w * 0.15f, bottom}, {cx + w, bottom}, {cx, top}, mid);
        g.fillTriangle({cx - w * 0.75f, bottom - 1.0f}, {cx + w * 0.75f, bottom - 1.0f}, {cx, bottom - 6.5f * s},
                       rgba(244, 248, 252, 230));
    }
    g.fillTriangle({cx - 3.5f * s, base - 39.0f * s}, {cx + 3.5f * s, base - 39.0f * s}, {cx, base - 46.0f * s},
                   rgba(250, 252, 255));
}

void drawCactus(Gfx& g, float cx, float cy, unsigned seed) {
    float s = 0.85f + h01(seed, 1) * 0.3f;
    cx += (h01(seed, 2) - 0.5f) * 10.0f;
    float base = cy + 16.0f * s;
    const Color body = rgba(70, 128, 70);
    const Color dark = rgba(44, 90, 50);
    const Color light = rgba(110, 168, 96);
    g.glow(cx + 6.0f, base, 16.0f * s, 6.0f * s, rgba(60, 30, 0, 120));
    g.fillRoundRect(cx - 6.0f * s, base - 36.0f * s, 12.0f * s, 36.0f * s, 6.0f * s, dark);
    g.fillRoundRect(cx - 5.0f * s, base - 35.0f * s, 9.0f * s, 34.0f * s, 4.5f * s, body);
    g.fillRoundRect(cx - 3.0f * s, base - 33.0f * s, 2.5f * s, 30.0f * s, 1.2f * s, light);
    bool left = h01(seed, 3) > 0.3f;
    bool right = h01(seed, 4) > 0.3f;
    if (left) {
        g.fillRoundRect(cx - 15.0f * s, base - 22.0f * s, 10.0f * s, 6.0f * s, 3.0f * s, dark);
        g.fillRoundRect(cx - 15.0f * s, base - 32.0f * s, 6.0f * s, 14.0f * s, 3.0f * s, body);
    }
    if (right) {
        g.fillRoundRect(cx + 4.0f * s, base - 17.0f * s, 11.0f * s, 6.0f * s, 3.0f * s, dark);
        g.fillRoundRect(cx + 9.0f * s, base - 27.0f * s, 6.0f * s, 14.0f * s, 3.0f * s, body);
    }
    if (h01(seed, 5) > 0.6f) g.fillCircle(cx, base - 37.0f * s, 3.0f * s, rgba(236, 110, 140));
}

void drawAutumnTree(Gfx& g, float cx, float cy, unsigned seed) {
    float s = 0.85f + h01(seed, 1) * 0.3f;
    cx += (h01(seed, 2) - 0.5f) * 8.0f;
    float base = cy + 18.0f * s;
    const Color leaves[4] = {rgba(204, 96, 34), rgba(226, 156, 44), rgba(170, 52, 30), rgba(190, 130, 40)};
    Color c = leaves[static_cast<int>(h01(seed, 3) * 4.0f) % 4];
    g.glow(cx + 4.0f, base, 18.0f * s, 6.0f * s, rgba(0, 0, 0, 120));
    g.fillRoundRect(cx - 3.0f * s, base - 18.0f * s, 6.0f * s, 18.0f * s, 2.0f, rgba(84, 56, 36));
    float top = base - 28.0f * s;
    g.fillCircle(cx - 8.0f * s, top + 4.0f * s, 11.0f * s, shade(c, 0.7f));
    g.fillCircle(cx + 8.0f * s, top + 4.0f * s, 11.0f * s, shade(c, 0.78f));
    g.fillCircle(cx, top - 4.0f * s, 13.0f * s, shade(c, 0.85f));
    g.fillCircle(cx - 4.0f * s, top - 6.0f * s, 9.0f * s, c);
    g.fillCircle(cx + 5.0f * s, top - 1.0f * s, 6.0f * s, shade(c, 1.12f));
}

}

const Theme& themeFor(int themeId) {
    if (themeId < 0 || themeId >= THEME_COUNT) return THEMES[0];
    return THEMES[themeId];
}

void drawFeature(Gfx& g, Feature f, float cx, float cy, unsigned seed, float time) {
    (void)time;
    switch (f) {
    case Feature::Bush:
        drawBush(g, cx, cy, seed);
        break;
    case Feature::Pine:
        drawPine(g, cx, cy, seed);
        break;
    case Feature::Cactus:
        drawCactus(g, cx, cy, seed);
        break;
    case Feature::AutumnTree:
        drawAutumnTree(g, cx, cy, seed);
        break;
    case Feature::LavaPool:
        break;
    }
}

float featureBase(Feature f) {
    switch (f) {
    case Feature::Bush:
        return 10.0f;
    case Feature::Pine:
        return 18.0f;
    case Feature::Cactus:
        return 16.0f;
    case Feature::AutumnTree:
        return 18.0f;
    case Feature::LavaPool:
        return 0.0f;
    }
    return 0.0f;
}
