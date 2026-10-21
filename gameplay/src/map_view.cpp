#include "map_view.h"
#include "cells.h"
#include "engine_core.h"
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace {

const float PI_F = 3.14159265358979f;

uint32_t hash2(int x, int y, uint32_t seed) {
    uint32_t h = static_cast<uint32_t>(x) * 374761393u + static_cast<uint32_t>(y) * 668265263u + seed * 2246822519u;
    h = (h ^ (h >> 13)) * 1274126177u;
    return h ^ (h >> 16);
}

float hash01(int x, int y, uint32_t seed) {
    return static_cast<float>(hash2(x, y, seed) & 0xFFFFFFu) / 16777216.0f;
}

float valueNoise(float x, float y, uint32_t seed) {
    float fx = std::floor(x);
    float fy = std::floor(y);
    int ix = static_cast<int>(fx);
    int iy = static_cast<int>(fy);
    float tx = x - fx;
    float ty = y - fy;
    tx = tx * tx * (3.0f - 2.0f * tx);
    ty = ty * ty * (3.0f - 2.0f * ty);
    float a = hash01(ix, iy, seed);
    float b = hash01(ix + 1, iy, seed);
    float c = hash01(ix, iy + 1, seed);
    float d = hash01(ix + 1, iy + 1, seed);
    return (a + (b - a) * tx) + ((c + (d - c) * tx) - (a + (b - a) * tx)) * ty;
}

float fbm(float x, float y, uint32_t seed) {
    return 0.55f * valueNoise(x / 52.0f, y / 52.0f, seed) + 0.3f * valueNoise(x / 17.0f, y / 17.0f, seed + 7) +
           0.15f * valueNoise(x / 5.5f, y / 5.5f, seed + 13);
}

float smoothstep(float e0, float e1, float v) {
    float t = std::clamp((v - e0) / (e1 - e0), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

void boxBlur(std::vector<float>& a, int w, int h, int r) {
    if (r < 1) return;
    std::vector<float> tmp(a.size());
    const float inv = 1.0f / (2 * r + 1);
    for (int pass = 0; pass < 3; pass++) {
        for (int y = 0; y < h; y++) {
            const float* row = &a[static_cast<size_t>(y) * w];
            float* out = &tmp[static_cast<size_t>(y) * w];
            float sum = 0.0f;
            for (int k = -r; k <= r; k++) sum += row[std::clamp(k, 0, w - 1)];
            for (int x = 0; x < w; x++) {
                out[x] = sum * inv;
                sum += row[std::min(x + r + 1, w - 1)] - row[std::max(x - r, 0)];
            }
        }
        for (int x = 0; x < w; x++) {
            float sum = 0.0f;
            for (int k = -r; k <= r; k++) sum += tmp[static_cast<size_t>(std::clamp(k, 0, h - 1)) * w + x];
            for (int y = 0; y < h; y++) {
                a[static_cast<size_t>(y) * w + x] = sum * inv;
                sum += tmp[static_cast<size_t>(std::min(y + r + 1, h - 1)) * w + x] -
                       tmp[static_cast<size_t>(std::max(y - r, 0)) * w + x];
            }
        }
    }
}

struct Rgb {
    float r, g, b;
};

Rgb toRgb(Color c) {
    return {static_cast<float>(c.r), static_cast<float>(c.g), static_cast<float>(c.b)};
}

Rgb lerp(Rgb a, Rgb b, float t) {
    return {a.r + (b.r - a.r) * t, a.g + (b.g - a.g) * t, a.b + (b.b - a.b) * t};
}

Rgb scaled(Rgb a, float k) {
    return {a.r * k, a.g * k, a.b * k};
}

uint32_t pack(Rgb c) {
    uint32_t r = static_cast<uint32_t>(std::clamp(c.r, 0.0f, 255.0f));
    uint32_t g = static_cast<uint32_t>(std::clamp(c.g, 0.0f, 255.0f));
    uint32_t b = static_cast<uint32_t>(std::clamp(c.b, 0.0f, 255.0f));
    return 0xFF000000u | (r << 16) | (g << 8) | b;
}

bool walkableTerrain(int t) {
    return t == CELL_PATH || t == CELL_SPAWN || t == CELL_GOAL;
}

std::vector<SDL_FPoint> boulderShape(float cx, float cy, float r, uint32_t seed, float squash) {
    std::vector<SDL_FPoint> pts;
    const int n = 9;
    for (int i = 0; i < n; i++) {
        float a = 2.0f * PI_F * i / n + hash01(i, 3, seed) * 0.3f;
        float rr = r * (0.84f + 0.16f * hash01(i, 7, seed));
        pts.push_back({cx + std::cos(a) * rr, cy + std::sin(a) * rr * squash});
    }
    return pts;
}

}

MapView::MapView() : terrain(nullptr), valid(false), builtScale(0.0f), spawn(-1), goal(-1) {
    for (int& t : types) t = -1;
}

MapView::~MapView() {
    if (terrain) SDL_DestroyTexture(terrain);
}

void MapView::invalidate() {
    valid = false;
}

void MapView::syncWithEngine(Gfx& g) {
    bool changed = !valid || std::fabs(builtScale - g.scale()) > 0.01f;
    for (int y = 0; y < MAP_H; y++) {
        for (int x = 0; x < MAP_W; x++) {
            int t = engine_cell_type(x, y);
            int& slot = types[cellIndex(x, y)];
            if (slot != t) {
                slot = t;
                changed = true;
            }
        }
    }
    spawn = engine_spawn_cell();
    goal = engine_goal_cell();
    if (changed) rebuildTerrain(g);
    rebuildRoute();
}

void MapView::rebuildRoute() {
    route.clear();
    if (spawn < 0 || goal < 0) return;
    int c = spawn;
    for (int i = 0; i < MAP_W * MAP_H; i++) {
        route.push_back({cellCentreX(c), cellCentreY(c)});
        if (c == goal) break;
        int n = engine_next_step(c);
        if (n < 0 || n == c) {
            route.clear();
            return;
        }
        c = n;
    }
}

void MapView::rebuildTerrain(Gfx& g) {
    const float s = std::min(g.scale(), 2.0f);
    const int W = static_cast<int>(std::lround(PANEL_X * s));
    const int H = static_cast<int>(std::lround(MAP_H * CELL_SIZE * s));
    const size_t N = static_cast<size_t>(W) * H;

    std::vector<float> path(N, 0.0f);
    std::vector<float> rock(N, 0.0f);
    for (int py = 0; py < H; py++) {
        int cy = std::min(MAP_H - 1, static_cast<int>(py / s) / CELL_SIZE);
        for (int px = 0; px < W; px++) {
            int cx = std::min(MAP_W - 1, static_cast<int>(px / s) / CELL_SIZE);
            int t = types[cellIndex(cx, cy)];
            size_t i = static_cast<size_t>(py) * W + px;
            if (walkableTerrain(t)) path[i] = 1.0f;
            if (t == CELL_BLOCKED) rock[i] = 1.0f;
        }
    }
    auto splat = [&](int cell, float radius) {
        if (cell < 0) return;
        float ccx = cellCentreX(cell) * s;
        float ccy = cellCentreY(cell) * s;
        float rr = radius * s;
        int x0 = std::max(0, static_cast<int>(ccx - rr));
        int x1 = std::min(W - 1, static_cast<int>(ccx + rr));
        int y0 = std::max(0, static_cast<int>(ccy - rr));
        int y1 = std::min(H - 1, static_cast<int>(ccy + rr));
        for (int py = y0; py <= y1; py++) {
            for (int px = x0; px <= x1; px++) {
                float dx = px - ccx;
                float dy = (py - ccy) * 1.25f;
                if (dx * dx + dy * dy <= rr * rr) path[static_cast<size_t>(py) * W + px] = 1.0f;
            }
        }
    };
    splat(goal, 44.0f);
    splat(spawn, 30.0f);

    boxBlur(path, W, H, static_cast<int>(7 * s));
    boxBlur(rock, W, H, static_cast<int>(9 * s));

    const Rgb grassDark = toRgb(pal::grassDark);
    const Rgb grassMid = toRgb(pal::grass);
    const Rgb grassLight = toRgb(pal::grassLight);
    const Rgb grassDry = {96, 108, 58};
    const Rgb dirtDark = toRgb(pal::dirtDark);
    const Rgb dirtLight = toRgb(pal::dirt);
    const Rgb gravel = {78, 82, 74};

    std::vector<uint32_t> pixels(N);
    for (int py = 0; py < H; py++) {
        float ly = py / s;
        for (int px = 0; px < W; px++) {
            float lx = px / s;
            size_t i = static_cast<size_t>(py) * W + px;
            float n1 = fbm(lx, ly, 11u);
            float n2 = fbm(lx + 300.0f, ly - 120.0f, 29u);
            float patch = valueNoise(lx / 150.0f, ly / 150.0f, 41u);

            float gn = std::clamp(0.2f + n1 * 0.7f, 0.0f, 1.0f);
            Rgb grass = gn < 0.5f ? lerp(grassDark, grassMid, gn * 2.0f) : lerp(grassMid, grassLight, (gn - 0.5f) * 2.0f);
            grass = lerp(grass, grassDry, smoothstep(0.55f, 0.9f, patch) * 0.45f);
            float speck = hash01(static_cast<int>(lx * 1.3f), static_cast<int>(ly * 1.3f), 5u);
            if (speck > 0.975f) grass = scaled(grass, 0.88f);
            else if (speck < 0.015f) grass = scaled(grass, 1.08f);

            Rgb dirt = lerp(dirtDark, dirtLight, std::clamp(n2 * 1.25f - 0.1f, 0.0f, 1.0f));
            float pebble = hash01(static_cast<int>(lx * 0.7f), static_cast<int>(ly * 0.7f), 17u);
            if (pebble > 0.975f) dirt = lerp(dirt, toRgb(pal::stoneLight), 0.45f);
            else if (pebble < 0.03f) dirt = scaled(dirt, 0.86f);

            float d = smoothstep(0.36f, 0.64f, path[i] + (n2 - 0.5f) * 0.18f);
            Rgb c = lerp(grass, dirt, d);
            float edge = d * (1.0f - d) * 4.0f;
            c = scaled(c, 1.0f - 0.2f * edge);

            float rk = smoothstep(0.15f, 0.85f, rock[i]);
            c = lerp(c, gravel, rk * 0.35f);

            float light = 1.04f - 0.08f * (ly / (MAP_H * CELL_SIZE)) - 0.04f * (lx / PANEL_X);
            pixels[i] = pack(scaled(c, light));
        }
    }

    const Rgb flowers[3] = {{222, 210, 150}, {204, 160, 184}, {214, 216, 210}};
    for (int cy = 0; cy < MAP_H; cy++) {
        for (int cx = 0; cx < MAP_W; cx++) {
            if (types[cellIndex(cx, cy)] != CELL_BUILDABLE) continue;
            uint32_t h = hash2(cx, cy, 77u);
            int clusters = (h % 5u == 0u) ? 1 : 0;
            for (int k = 0; k < clusters; k++) {
                float fx = cx * CELL_SIZE + 6.0f + hash01(cx * 7 + k, cy, 3u) * (CELL_SIZE - 12.0f);
                float fy = cy * CELL_SIZE + 6.0f + hash01(cx, cy * 7 + k, 9u) * (CELL_SIZE - 12.0f);
                Rgb col = flowers[hash2(cx + k, cy, 13u) % 3u];
                for (int f = 0; f < 5; f++) {
                    float ox = fx + (hash01(f, k, h) - 0.5f) * 9.0f;
                    float oy = fy + (hash01(k, f, h) - 0.5f) * 9.0f;
                    int pcx = static_cast<int>(ox * s);
                    int pcy = static_cast<int>(oy * s);
                    int rad = std::max(1, static_cast<int>(1.1f * s));
                    for (int yy = -rad; yy <= rad; yy++) {
                        for (int xx = -rad; xx <= rad; xx++) {
                            if (xx * xx + yy * yy > rad * rad) continue;
                            int qx = pcx + xx;
                            int qy = pcy + yy;
                            if (qx < 0 || qy < 0 || qx >= W || qy >= H) continue;
                            size_t i = static_cast<size_t>(qy) * W + qx;
                            if (path[i] > 0.15f || rock[i] > 0.15f) continue;
                            pixels[i] = pack(col);
                        }
                    }
                }
            }
        }
    }

    if (terrain) SDL_DestroyTexture(terrain);
    terrain = SDL_CreateTexture(g.sdl(), SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, W, H);
    if (terrain) {
        SDL_UpdateTexture(terrain, nullptr, pixels.data(), W * static_cast<int>(sizeof(uint32_t)));
        SDL_SetTextureBlendMode(terrain, SDL_BLENDMODE_NONE);
    }
    builtScale = g.scale();
    valid = true;
}

void MapView::drawGround(Gfx& g) const {
    if (terrain) g.drawTexture(terrain, 0.0f, 0.0f, static_cast<float>(PANEL_X), static_cast<float>(MAP_H * CELL_SIZE));
    else g.fillRect(0, 0, PANEL_X, MAP_H * CELL_SIZE, pal::grass);
}

void MapView::drawGrid(Gfx& g, float alpha) const {
    if (alpha <= 0.01f) return;
    Color c = rgba(255, 255, 255, static_cast<int>(28 * alpha));
    for (int x = 1; x < MAP_W; x++) g.line(x * CELL_SIZE, 0, x * CELL_SIZE, MAP_H * CELL_SIZE, 1.0f, c);
    for (int y = 1; y < MAP_H; y++) g.line(0, y * CELL_SIZE, PANEL_X, y * CELL_SIZE, 1.0f, c);
}

void MapView::drawRoute(Gfx& g) const {
    if (route.size() < 2) return;
    std::vector<float> acc(route.size(), 0.0f);
    for (size_t i = 1; i < route.size(); i++) {
        float dx = route[i].x - route[i - 1].x;
        float dy = route[i].y - route[i - 1].y;
        acc[i] = acc[i - 1] + std::sqrt(dx * dx + dy * dy);
    }
    const float total = acc.back();
    const float gap = 15.0f;
    float offset = std::fmod(g.time() * 26.0f, gap);
    size_t seg = 1;
    for (float d = offset; d < total; d += gap) {
        while (seg < route.size() - 1 && acc[seg] < d) seg++;
        float segLen = acc[seg] - acc[seg - 1];
        float t = segLen > 0.0f ? (d - acc[seg - 1]) / segLen : 0.0f;
        float px = route[seg - 1].x + (route[seg].x - route[seg - 1].x) * t;
        float py = route[seg - 1].y + (route[seg].y - route[seg - 1].y) * t;
        float fade = std::min(1.0f, std::min(d, total - d) / 40.0f);
        g.fillCircle(px, py, 2.1f, rgba(255, 238, 200, static_cast<int>(95 * fade)));
    }
}

void MapView::drawRocks(Gfx& g) const {
    for (int cy = 0; cy < MAP_H; cy++) {
        for (int cx = 0; cx < MAP_W; cx++) {
            if (types[cellIndex(cx, cy)] != CELL_BLOCKED) continue;
            uint32_t seed = hash2(cx, cy, 101u);
            float bx = cx * CELL_SIZE + CELL_SIZE * 0.5f;
            float by = cy * CELL_SIZE + CELL_SIZE * 0.5f;
            struct B {
                float x, y, r;
            };
            B list[3] = {
                {bx + (hash01(1, 1, seed) - 0.5f) * 8.0f, by + (hash01(2, 1, seed) - 0.5f) * 6.0f, 15.0f + hash01(3, 1, seed) * 4.0f},
                {bx + (hash01(4, 1, seed) > 0.5f ? 13.0f : -13.0f), by + 10.0f, 7.0f + hash01(5, 1, seed) * 3.0f},
                {bx + (hash01(6, 1, seed) > 0.5f ? -14.0f : 14.0f), by - 9.0f, 5.0f + hash01(7, 1, seed) * 2.0f},
            };
            for (int k = 0; k < 3; k++) {
                g.glow(list[k].x + 3.0f, list[k].y + list[k].r * 0.55f, list[k].r * 1.3f, list[k].r * 0.6f, rgba(0, 0, 0, 120));
            }
            for (int k = 2; k >= 0; k--) {
                const B& b = list[k];
                uint32_t sd = seed + k * 17u;
                g.fillPolygon(boulderShape(b.x, b.y, b.r, sd, 0.82f), shade(pal::stoneDark, 0.85f));
                g.fillPolygonGradient(boulderShape(b.x - b.r * 0.08f, b.y - b.r * 0.14f, b.r * 0.86f, sd, 0.78f),
                                      shade(pal::stone, 1.08f), shade(pal::stone, 0.82f));
                g.fillPolygon(boulderShape(b.x - b.r * 0.28f, b.y - b.r * 0.32f, b.r * 0.4f, sd + 1u, 0.7f),
                              withAlpha(pal::stoneLight, 150));
                if (k == 0) {
                    g.line(b.x + b.r * 0.1f, b.y - b.r * 0.2f, b.x + b.r * 0.35f, b.y + b.r * 0.25f, 1.2f,
                           withAlpha(pal::stoneDark, 200));
                    if (hash01(9, 9, seed) > 0.4f) {
                        g.fillEllipse(b.x + b.r * 0.2f, b.y + b.r * 0.42f, b.r * 0.45f, b.r * 0.18f, rgba(84, 116, 62, 170));
                    }
                }
            }
        }
    }
}

void MapView::drawPortal(Gfx& g) const {
    if (spawn < 0) return;
    float cx = cellCentreX(spawn);
    float cy = cellCentreY(spawn);
    float t = g.time();
    float pulse = 0.5f + 0.5f * std::sin(t * 2.4f);
    g.glow(cx, cy, 34.0f + pulse * 4.0f, 30.0f + pulse * 4.0f, rgba(150, 70, 220, static_cast<int>(70 + pulse * 40)));
    g.fillEllipse(cx, cy + 2.0f, 19.0f, 16.0f, rgba(26, 12, 36));
    g.fillCircleGradient(cx, cy + 1.0f, 16.0f, rgba(210, 150, 255, 230), rgba(60, 20, 90, 230));
    g.fillEllipse(cx, cy + 1.0f, 9.0f, 8.0f, rgba(20, 6, 30, 230));
    for (int i = 0; i < 3; i++) {
        float r = 6.0f + i * 4.0f;
        float a = t * (2.4f - i * 0.6f) * (i % 2 == 0 ? 1.0f : -1.0f) + i * 1.7f;
        g.strokeArc(cx, cy + 1.0f, r, a, a + 2.2f, 1.6f, rgba(220, 170, 255, 190 - i * 40));
    }
    for (int i = 0; i < 9; i++) {
        float a = i * 2.0f * PI_F / 9.0f;
        float sx = cx + std::cos(a) * 21.0f;
        float sy = cy + 2.0f + std::sin(a) * 18.0f;
        float r = 4.2f + hash01(i, 2, 55u) * 1.6f;
        g.glow(sx + 1.0f, sy + 2.5f, r * 1.2f, r * 0.6f, rgba(0, 0, 0, 110));
        g.fillCircleGradient(sx, sy, r, pal::stoneLight, pal::stoneDark);
    }
}

void MapView::drawCastle(Gfx& g, float hitFlash) const {
    if (goal < 0) return;
    const float cx = cellCentreX(goal);
    const float base = cellCentreY(goal) + 20.0f;
    const Color roof = rgba(66, 86, 134);
    const Color roofDark = rgba(42, 56, 92);
    const Color mortar = withAlpha(pal::stoneDark, 140);

    g.glow(cx + 6.0f, base - 1.0f, 52.0f, 15.0f, rgba(0, 0, 0, 140));

    float keepL = cx - 13.0f;
    float keepR = cx + 13.0f;
    float keepTop = base - 64.0f;
    g.fillRectGradient(keepL, keepTop, keepR - keepL, 40.0f, shade(pal::stone, 1.12f), shade(pal::stone, 0.8f));
    for (int i = 0; i < 4; i++) g.fillRect(keepL + i * 7.5f, keepTop - 5.0f, 4.5f, 5.0f, shade(pal::stone, 1.05f));
    g.fillRoundRect(cx - 3.0f, keepTop + 10.0f, 6.0f, 10.0f, 3.0f, rgba(30, 26, 34));

    float poleTop = keepTop - 24.0f;
    g.line(cx, keepTop - 5.0f, cx, poleTop, 1.6f, rgba(70, 60, 50));
    std::vector<SDL_FPoint> flag;
    const int fs = 8;
    for (int i = 0; i <= fs; i++) {
        float fx = cx + 1.0f + i * 2.4f;
        float wave = std::sin(g.time() * 5.0f - i * 0.7f) * 1.6f * (i / static_cast<float>(fs));
        flag.push_back({fx, poleTop + wave});
    }
    for (int i = fs; i >= 0; i--) {
        float fx = cx + 1.0f + i * 2.4f;
        float wave = std::sin(g.time() * 5.0f - i * 0.7f) * 1.6f * (i / static_cast<float>(fs));
        flag.push_back({fx, poleTop + 10.0f + wave - i * 0.35f});
    }
    for (size_t i = 0; i + 1 < flag.size() / 2; i++) {
        size_t j = flag.size() - 1 - i;
        std::vector<SDL_FPoint> q = {flag[i], flag[i + 1], flag[j - 1], flag[j]};
        g.fillPolygon(q, i % 2 == 0 ? pal::gold : shade(pal::gold, 0.92f));
    }

    float wallL = cx - 31.0f;
    float wallR = cx + 31.0f;
    float wallTop = base - 34.0f;
    g.fillRectGradient(wallL, wallTop, wallR - wallL, 34.0f, shade(pal::stone, 1.08f), shade(pal::stone, 0.72f));
    for (int row = 1; row < 5; row++) {
        float y = wallTop + row * 7.0f;
        g.line(wallL, y, wallR, y, 0.8f, mortar);
        for (float x = wallL + (row % 2 ? 4.0f : 8.0f); x < wallR; x += 9.0f) g.line(x, y - 7.0f, x, y, 0.8f, mortar);
    }
    for (int i = 0; i < 7; i++) {
        float mx = wallL + 1.0f + i * 9.0f;
        g.fillRect(mx, wallTop - 6.0f, 6.0f, 6.0f, shade(pal::stone, 1.1f));
        g.fillRect(mx, wallTop - 6.0f, 6.0f, 1.5f, shade(pal::stone, 1.25f));
    }
    g.fillRoundRect(cx - 9.0f, base - 22.0f, 18.0f, 26.0f, 9.0f, rgba(28, 22, 22));
    for (int i = 0; i < 4; i++) g.line(cx - 6.0f + i * 4.0f, base - 19.0f, cx - 6.0f + i * 4.0f, base, 1.0f, rgba(110, 106, 100));
    g.line(cx - 8.0f, base - 12.0f, cx + 8.0f, base - 12.0f, 1.0f, rgba(110, 106, 100));

    for (int side = -1; side <= 1; side += 2) {
        float tx = cx + side * 31.0f;
        float tL = tx - 9.5f;
        float tTop = base - 48.0f;
        g.fillRectGradient(tL, tTop, 19.0f, 48.0f, shade(pal::stone, 1.15f), shade(pal::stone, 0.7f));
        g.fillRect(tL, tTop, 3.0f, 48.0f, withAlpha(rgba(255, 255, 255), 30));
        g.fillRoundRect(tx - 2.5f, tTop + 14.0f, 5.0f, 9.0f, 2.5f, rgba(30, 26, 34));
        g.fillTriangle({tL - 3.0f, tTop + 1.0f}, {tL + 22.0f, tTop + 1.0f}, {tx, tTop - 24.0f}, roof);
        g.fillTriangle({tx, tTop + 1.0f}, {tL + 22.0f, tTop + 1.0f}, {tx, tTop - 24.0f}, roofDark);
        g.fillCircle(tx, tTop - 25.0f, 1.8f, pal::gold);
    }

    if (hitFlash > 0.0f) {
        g.glow(cx, base - 26.0f, 58.0f, 48.0f, rgba(255, 60, 40, static_cast<int>(170 * std::min(1.0f, hitFlash / 0.3f))));
    }
}

void MapView::drawVignette(Gfx& g) const {
    const float W = PANEL_X;
    const float H = MAP_H * CELL_SIZE;
    const Color dark = rgba(4, 8, 6, 120);
    const Color clear = rgba(4, 8, 6, 0);
    const float e = 70.0f;
    g.fillPolygonColors({{0, 0}, {W, 0}, {W, e}, {0, e}}, {dark, dark, clear, clear});
    g.fillPolygonColors({{0, H - e}, {W, H - e}, {W, H}, {0, H}}, {clear, clear, dark, dark});
    g.fillPolygonColors({{0, 0}, {e, 0}, {e, H}, {0, H}}, {dark, clear, clear, dark});
    g.fillPolygonColors({{W - e * 0.6f, 0}, {W, 0}, {W, H}, {W - e * 0.6f, H}}, {clear, dark, dark, clear});
}
