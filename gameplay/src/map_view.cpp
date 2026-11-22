#include "map_view.h"
#include "cells.h"
#include "engine_core.h"
#include "theme.h"
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

Color litFace(Color c, float nx, float ny) {
    return shade(c, std::clamp(0.76f + 0.2f * ny - 0.22f * nx, 0.5f, 1.08f));
}

std::vector<SDL_FPoint> rectPts(float x0, float y0, float x1, float y1) {
    return {{x1, y0}, {x1, y1}, {x0, y1}, {x0, y0}};
}

std::vector<SDL_FPoint> ringPts(float cx, float cy, float r, int n) {
    std::vector<SDL_FPoint> pts;
    for (int i = 0; i < n; i++) {
        float a = 2.0f * PI_F * i / n;
        pts.push_back({cx + std::cos(a) * r, cy + std::sin(a) * r});
    }
    return pts;
}

void extrude(Gfx& g, const std::vector<SDL_FPoint>& pts, float z0, float z1, Color side, Color top) {
    g.useScreen();
    const size_t n = pts.size();
    for (size_t i = 0; i < n; i++) {
        SDL_FPoint a = pts[i];
        SDL_FPoint b = pts[(i + 1) % n];
        float ex = b.x - a.x;
        float ey = b.y - a.y;
        float len = std::sqrt(ex * ex + ey * ey);
        if (len < 0.001f) continue;
        float nx = ey / len;
        float ny = -ex / len;
        if (!g.facesViewer(nx, ny)) continue;
        Color c = litFace(side, nx, ny);
        SDL_FPoint b0 = g.project(a.x, a.y, z0);
        SDL_FPoint b1 = g.project(b.x, b.y, z0);
        SDL_FPoint t0 = g.project(a.x, a.y, z1);
        SDL_FPoint t1 = g.project(b.x, b.y, z1);
        g.fillPolygonColors({t0, t1, b1, b0}, {shade(c, 1.05f), shade(c, 1.05f), shade(c, 0.78f), shade(c, 0.78f)});
    }
    std::vector<SDL_FPoint> lid;
    for (const SDL_FPoint& p : pts) lid.push_back(g.project(p.x, p.y, z1));
    g.fillPolygon(lid, top);
}

void cone(Gfx& g, float cx, float cy, float r, float z, float h, Color light, Color dark) {
    g.useScreen();
    const int n = 16;
    SDL_FPoint apex = g.project(cx, cy, z + h);
    for (int pass = 0; pass < 2; pass++) {
        for (int i = 0; i < n; i++) {
            float a0 = 2.0f * PI_F * i / n;
            float a1 = 2.0f * PI_F * (i + 1) / n;
            float am = (a0 + a1) * 0.5f;
            bool front = g.facesViewer(std::cos(am), std::sin(am));
            if (front != (pass == 1)) continue;
            SDL_FPoint p0 = g.project(cx + std::cos(a0) * r, cy + std::sin(a0) * r, z);
            SDL_FPoint p1 = g.project(cx + std::cos(a1) * r, cy + std::sin(a1) * r, z);
            Color c = front ? litFace(light, std::cos(am), std::sin(am)) : dark;
            g.fillPolygonColors({apex, p0, p1}, {shade(c, 1.15f), c, c});
        }
    }
}

void castleTower(Gfx& g, float x, float y, float r, float h, Color stoneCol, Color roof, Color roofDark) {
    extrude(g, ringPts(x, y, r, 18), 0.0f, h, stoneCol, shade(stoneCol, 1.1f));
    for (int i = 0; i < 8; i++) {
        float a = PI_F * 0.08f + i * PI_F * 0.12f;
        float mx = x + std::cos(a) * (r - 1.5f);
        float my = y + std::sin(a) * (r - 1.5f);
        extrude(g, rectPts(mx - 1.6f, my - 1.6f, mx + 1.6f, my + 1.6f), h, h + 4.0f, stoneCol, shade(stoneCol, 1.2f));
    }
    SDL_FPoint s0 = g.project(x - 1.3f, y + r, h * 0.45f);
    SDL_FPoint s1 = g.project(x + 1.3f, y + r, h * 0.62f);
    g.fillRoundRect(s0.x, s1.y, s1.x - s0.x, s0.y - s1.y, 1.2f, rgba(30, 26, 34));
    cone(g, x, y, r + 2.5f, h + 4.0f, 24.0f, roof, roofDark);
    SDL_FPoint tip = g.project(x, y, h + 28.0f);
    g.fillCircle(tip.x, tip.y - 1.0f, 1.8f, pal::gold);
}

}

MapView::MapView() : terrain(nullptr), theme(-1), valid(false), builtScale(0.0f), goal(-1) {
    for (int& t : types) t = -1;
    for (bool& f : features) f = false;
}

MapView::~MapView() {
    if (terrain) SDL_DestroyTexture(terrain);
}

void MapView::invalidate() {
    valid = false;
}

void MapView::syncWithEngine(Gfx& g) {
    bool changed = !valid || std::fabs(builtScale - g.scale()) > 0.01f;
    int level = engine_current_level();
    int newTheme = engine_level_theme(level);
    if (newTheme != theme) {
        theme = newTheme;
        changed = true;
    }
    for (int y = 0; y < MAP_H; y++) {
        for (int x = 0; x < MAP_W; x++) {
            int t = engine_cell_type(x, y);
            int& slot = types[cellIndex(x, y)];
            if (slot != t) {
                slot = t;
                changed = true;
            }
            bool f = engine_level_feature(level, x, y) != 0;
            if (features[cellIndex(x, y)] != f) {
                features[cellIndex(x, y)] = f;
                changed = true;
            }
        }
    }
    spawns.clear();
    for (int i = 0; i < engine_spawn_count(); i++) spawns.push_back(engine_spawn_at(i));
    goal = engine_goal_cell();
    if (changed) rebuildTerrain(g);
}

void MapView::rebuildTerrain(Gfx& g) {
    const float s = std::min(g.scale(), 2.0f);
    const int W = static_cast<int>(std::lround(PANEL_X * s));
    const int H = static_cast<int>(std::lround(MAP_H * CELL_SIZE * s));
    const size_t N = static_cast<size_t>(W) * H;

    const Theme& th = themeFor(theme);
    const bool lavaTheme = th.feature == Feature::LavaPool;
    std::vector<float> path(N, 0.0f);
    std::vector<float> rock(N, 0.0f);
    std::vector<float> lava(N, 0.0f);
    std::vector<float> tile(N, 0.0f);
    for (int py = 0; py < H; py++) {
        int cy = std::min(MAP_H - 1, static_cast<int>(py / s) / CELL_SIZE);
        float fy = std::fmod(py / s, static_cast<float>(CELL_SIZE));
        for (int px = 0; px < W; px++) {
            int cx = std::min(MAP_W - 1, static_cast<int>(px / s) / CELL_SIZE);
            float fx = std::fmod(px / s, static_cast<float>(CELL_SIZE));
            int t = types[cellIndex(cx, cy)];
            size_t i = static_cast<size_t>(py) * W + px;
            if (walkableTerrain(t)) path[i] = 1.0f;
            if (t == CELL_BLOCKED) {
                if (lavaTheme && features[cellIndex(cx, cy)]) lava[i] = 1.0f;
                else rock[i] = 1.0f;
            }
            if (t == CELL_BUILDABLE) {
                float edge = std::min(std::min(fx, CELL_SIZE - fx), std::min(fy, CELL_SIZE - fy));
                tile[i] = ((cx + cy) % 2 == 0 ? 0.035f : -0.035f) - (edge < 1.2f ? 0.07f : 0.0f);
            }
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
    for (int s : spawns) splat(s, 30.0f);

    boxBlur(path, W, H, static_cast<int>(7 * s));
    boxBlur(rock, W, H, static_cast<int>(9 * s));
    if (lavaTheme) boxBlur(lava, W, H, static_cast<int>(6 * s));
    const Rgb grassDark = toRgb(th.groundDark);
    const Rgb grassMid = toRgb(th.groundMid);
    const Rgb grassLight = toRgb(th.groundLight);
    const Rgb grassDry = toRgb(th.groundPatch);
    const Rgb dirtDark = toRgb(th.pathDark);
    const Rgb dirtLight = toRgb(th.pathLight);
    const Rgb gravel = toRgb(th.gravel);
    const Rgb pebbleLight = toRgb(th.rockLight);

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
            if (pebble > 0.975f) dirt = lerp(dirt, pebbleLight, 0.45f);
            else if (pebble < 0.03f) dirt = scaled(dirt, 0.86f);

            float d = smoothstep(0.36f, 0.64f, path[i] + (n2 - 0.5f) * 0.18f);
            Rgb c = lerp(grass, dirt, d);
            float edge = d * (1.0f - d) * 4.0f;
            c = scaled(c, 1.0f - 0.2f * edge);

            float rk = smoothstep(0.15f, 0.85f, rock[i]);
            c = lerp(c, gravel, rk * 0.35f);

            c = scaled(c, 1.0f + tile[i] * (1.0f - d));

            if (lavaTheme && lava[i] > 0.02f) {
                float flow = valueNoise(lx / 22.0f + ly / 60.0f, ly / 22.0f, 93u);
                float plate = valueNoise(lx / 13.0f, ly / 13.0f, 94u);
                float m = lava[i] + (n1 - 0.5f) * 0.22f;
                float bank = smoothstep(0.18f, 0.36f, m);
                float molten = smoothstep(0.42f, 0.62f, m);
                Rgb crust = {28, 20, 20};
                Rgb deep = {176, 40, 12};
                Rgb hot = {250, 128, 32};
                Rgb white = {255, 222, 120};
                Rgb lavaCol = lerp(deep, hot, smoothstep(0.3f, 0.75f, flow));
                lavaCol = lerp(lavaCol, white, smoothstep(0.78f, 0.95f, flow) * molten);
                if (plate > 0.66f && molten > 0.5f) lavaCol = lerp(lavaCol, {70, 34, 24}, smoothstep(0.66f, 0.72f, plate) * 0.85f);
                c = lerp(c, crust, bank);
                c = lerp(c, lavaCol, molten);
            } else if (lavaTheme && lava[i] > 0.0f) {
                c = lerp(c, {40, 26, 22}, std::min(1.0f, lava[i] * 6.0f));
            }

            float light = 1.04f - 0.08f * (ly / (MAP_H * CELL_SIZE)) - 0.04f * (lx / PANEL_X);
            pixels[i] = pack(scaled(c, light));
        }
    }

    const Rgb flowers[3] = {toRgb(th.specks[0]), toRgb(th.specks[1]), toRgb(th.specks[2])};
    const uint32_t every = static_cast<uint32_t>(th.speckEvery > 0 ? th.speckEvery : 5);
    for (int cy = 0; cy < MAP_H; cy++) {
        for (int cx = 0; cx < MAP_W; cx++) {
            if (types[cellIndex(cx, cy)] != CELL_BUILDABLE) continue;
            uint32_t h = hash2(cx, cy, 77u);
            int clusters = (h % every == 0u) ? 1 : 0;
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
                            if (path[i] > 0.15f || rock[i] > 0.15f || lava[i] > 0.05f) continue;
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

IsoView MapView::battlefieldView() {
    return {0.0f, 0.0f, 25.0f, 23.5f, 1.0f, true};
}

void MapView::drawBackdrop(Gfx& g) const {
    const Theme& th = themeFor(theme);
    const float H = MAP_H * CELL_SIZE;
    g.useScreen();
    g.fillRectGradient(0.0f, 0.0f, PANEL_X, H, th.skyTop, th.skyBottom);
    for (int i = 0; i < 5; i++) {
        float y = 70.0f + i * 120.0f;
        g.glow(PANEL_X * 0.5f, y, PANEL_X * 0.7f, 26.0f, withAlpha(shade(th.skyTop, 1.25f), 22));
    }
    SDL_FPoint bottom = g.project(MAP_W * CELL_SIZE, MAP_H * CELL_SIZE, -ISLAND_DEPTH);
    SDL_FPoint mid = g.project(MAP_W * CELL_SIZE * 0.5f, MAP_H * CELL_SIZE * 0.5f);
    g.glow(mid.x, bottom.y - 30.0f, 430.0f, 70.0f, rgba(0, 0, 0, 120));
}

void MapView::drawGround(Gfx& g) const {
    const Theme& th = themeFor(theme);
    const float W = MAP_W * CELL_SIZE;
    const float H = MAP_H * CELL_SIZE;
    const float D = ISLAND_DEPTH;
    g.useScreen();
    SDL_FPoint l0 = g.project(0.0f, H);
    SDL_FPoint b0 = g.project(W, H);
    SDL_FPoint r0 = g.project(W, 0.0f);
    SDL_FPoint l1 = g.project(0.0f, H, -D);
    SDL_FPoint b1 = g.project(W, H, -D);
    SDL_FPoint r1 = g.project(W, 0.0f, -D);
    g.fillPolygonColors({l0, b0, b1, l1}, {th.cliffTop, th.cliffTop, th.cliffBottom, th.cliffBottom});
    g.fillPolygonColors({b0, r0, r1, b1}, {shade(th.cliffTop, 0.72f), shade(th.cliffTop, 0.72f), shade(th.cliffBottom, 0.7f),
                                           shade(th.cliffBottom, 0.7f)});
    for (int k = 1; k < 4; k++) {
        float z = -D * k / 4.0f;
        Color strata = withAlpha(shade(th.cliffBottom, 0.6f), 90);
        SDL_FPoint a = g.project(0.0f, H, z);
        SDL_FPoint b = g.project(W, H, z);
        SDL_FPoint c = g.project(W, 0.0f, z);
        g.line(a.x, a.y, b.x, b.y, 1.0f, strata);
        g.line(b.x, b.y, c.x, c.y, 1.0f, strata);
    }
    for (int i = 1; i < MAP_W; i++) {
        SDL_FPoint a = g.project(i * CELL_SIZE, H);
        SDL_FPoint b = g.project(i * CELL_SIZE, H, -D);
        g.line(a.x, a.y, b.x, b.y, 1.0f, rgba(0, 0, 0, 40));
    }
    for (int j = 1; j < MAP_H; j++) {
        SDL_FPoint a = g.project(W, j * CELL_SIZE);
        SDL_FPoint b = g.project(W, j * CELL_SIZE, -D);
        g.line(a.x, a.y, b.x, b.y, 1.0f, rgba(0, 0, 0, 40));
    }

    g.useGround(0.0f);
    if (terrain) g.drawTexture(terrain, 0.0f, 0.0f, W, H);
    else g.fillRect(0, 0, W, H, th.groundMid);
    g.useScreen();
    g.line(l0.x, l0.y, b0.x, b0.y, 1.4f, rgba(255, 255, 255, 50));
    g.line(b0.x, b0.y, r0.x, r0.y, 1.4f, rgba(255, 255, 255, 30));
}

void MapView::drawGrid(Gfx& g, float alpha) const {
    if (alpha <= 0.01f) return;
    Color c = rgba(255, 255, 255, static_cast<int>(40 * alpha));
    g.useGround(0.0f);
    for (int x = 1; x < MAP_W; x++) g.line(x * CELL_SIZE, 0, x * CELL_SIZE, MAP_H * CELL_SIZE, 1.6f, c);
    for (int y = 1; y < MAP_H; y++) g.line(0, y * CELL_SIZE, MAP_W * CELL_SIZE, y * CELL_SIZE, 1.6f, c);
    g.useScreen();
}

void MapView::drawLava(Gfx& g) const {
    if (themeFor(theme).feature != Feature::LavaPool) return;
    const float t = g.time();
    for (int cy = 0; cy < MAP_H; cy++) {
        for (int cx = 0; cx < MAP_W; cx++) {
            int c = cellIndex(cx, cy);
            if (!features[c]) continue;
            float wx = cx * CELL_SIZE + CELL_SIZE * 0.5f;
            float wy = cy * CELL_SIZE + CELL_SIZE * 0.5f;
            float pulse = 0.5f + 0.5f * std::sin(t * 1.7f + cx * 0.9f + cy * 1.3f);
            g.useGround(0.0f);
            g.glow(wx, wy, 34.0f, 34.0f, rgba(255, 120, 30, static_cast<int>(30 + 40 * pulse)));
            float phase = std::fmod(t * 0.6f + hash01(cx, cy, 71u) * 3.0f, 3.0f);
            if (phase < 1.0f) {
                float bx = wx + (hash01(cx, cy, 72u) - 0.5f) * 26.0f;
                float by = wy + (hash01(cx, cy, 73u) - 0.5f) * 26.0f;
                g.useBillboard(bx, by, 0.0f);
                float r = 1.5f + phase * 3.0f;
                g.fillCircle(bx, by - phase * 3.0f, r, rgba(255, 210, 110, static_cast<int>(220 * (1.0f - phase))));
                g.strokeCircle(bx, by - phase * 3.0f, r, 1.0f, rgba(120, 30, 10, static_cast<int>(200 * (1.0f - phase))));
            }
        }
    }
    g.useScreen();
}

bool MapView::hasCellObject(int cell) const {
    if (cell < 0 || cell >= MAP_W * MAP_H || types[cell] != CELL_BLOCKED) return false;
    return !(features[cell] && themeFor(theme).feature == Feature::LavaPool);
}

void MapView::drawCellObject(Gfx& g, int cell) const {
    if (!hasCellObject(cell)) return;
    const Theme& th = themeFor(theme);
    const int cx = cellX(cell);
    const int cy = cellY(cell);
    uint32_t seed = hash2(cx, cy, 101u);
    const float wx = cx * CELL_SIZE + CELL_SIZE * 0.5f;
    const float wy = cy * CELL_SIZE + CELL_SIZE * 0.5f;
    g.useGround(0.0f);
    g.glow(wx + 4.0f, wy + 4.0f, 26.0f, 26.0f, rgba(0, 0, 0, 90));
    int neighbours = 0;
    if (cx > 0 && features[cell - 1]) neighbours++;
    if (cx < MAP_W - 1 && features[cell + 1]) neighbours++;
    if (cy > 0 && features[cell - MAP_W]) neighbours++;
    if (cy < MAP_H - 1 && features[cell + MAP_W]) neighbours++;
    const bool sparse = features[cell] && neighbours >= 2 && hash01(cx, cy, 211u) < 0.5f;
    if (features[cell] && !sparse) {
        g.useSprite(wx, wy, 0.0f, 1.0f, wx, wy + featureBase(th.feature));
        drawFeature(g, th.feature, wx, wy, seed, g.time());
        g.useScreen();
        return;
    }
    const float bx = wx;
    const float by = wy;
    g.useSprite(wx, wy, 0.0f, 1.0f, bx, by + 12.0f);
    struct B {
        float x, y, r;
    };
    B list[3] = {
        {bx + (hash01(1, 1, seed) - 0.5f) * 8.0f, by + (hash01(2, 1, seed) - 0.5f) * 4.0f, 15.0f + hash01(3, 1, seed) * 4.0f},
        {bx + (hash01(4, 1, seed) > 0.5f ? 13.0f : -13.0f), by + 9.0f, 7.0f + hash01(5, 1, seed) * 3.0f},
        {bx + (hash01(6, 1, seed) > 0.5f ? -14.0f : 14.0f), by + 4.0f, 5.0f + hash01(7, 1, seed) * 2.0f},
    };
    const int count = sparse ? 1 : 2;
    if (sparse) list[0].r = 10.0f + hash01(3, 1, seed) * 3.0f;
    for (int k = 0; k < count; k++) {
        g.glow(list[k].x + 3.0f, list[k].y + list[k].r * 0.55f, list[k].r * 1.3f, list[k].r * 0.5f, rgba(0, 0, 0, 120));
    }
    for (int k = 0; k < count; k++) {
        const B& b = list[k];
        uint32_t sd = seed + k * 17u;
        g.fillPolygon(boulderShape(b.x, b.y, b.r, sd, 0.9f), shade(th.rockDark, 0.85f));
        g.fillPolygonGradient(boulderShape(b.x - b.r * 0.1f, b.y - b.r * 0.22f, b.r * 0.84f, sd, 0.82f),
                              shade(th.rockMid, 1.12f), shade(th.rockMid, 0.78f));
        g.fillPolygon(boulderShape(b.x - b.r * 0.3f, b.y - b.r * 0.42f, b.r * 0.38f, sd + 1u, 0.7f), withAlpha(th.rockLight, 160));
        if (k == 0) {
            g.line(b.x + b.r * 0.1f, b.y - b.r * 0.2f, b.x + b.r * 0.35f, b.y + b.r * 0.25f, 1.2f, withAlpha(th.rockDark, 200));
            if (th.rockCap.a > 0 && th.feature == Feature::Pine) {
                g.fillPolygon(boulderShape(b.x - b.r * 0.05f, b.y - b.r * 0.55f, b.r * 0.62f, sd + 2u, 0.42f), th.rockCap);
            } else if (th.rockCap.a > 0 && hash01(9, 9, seed) > 0.4f) {
                g.fillEllipse(b.x + b.r * 0.2f, b.y + b.r * 0.42f, b.r * 0.45f, b.r * 0.18f, th.rockCap);
            } else if (th.feature == Feature::LavaPool) {
                g.glow(b.x, b.y + b.r * 0.7f, b.r * 1.2f, b.r * 0.35f, rgba(255, 110, 30, 70));
            }
        }
    }
    g.useScreen();
}

void MapView::drawPortals(Gfx& g) const {
    for (int s : spawns) drawPortal(g, s);
    g.useScreen();
}

void MapView::drawPortal(Gfx& g, int cell) const {
    if (cell < 0) return;
    float cx = cellCentreX(cell);
    float cy = cellCentreY(cell);
    float t = g.time();
    float pulse = 0.5f + 0.5f * std::sin(t * 2.4f);
    g.useGround(0.0f);
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
    const float cy = cellCentreY(goal);
    const Color wall = pal::stone;
    const Color wallTop = shade(pal::stone, 1.18f);
    const Color keepCol = rgba(146, 142, 136);
    const Color roof = rgba(66, 86, 134);
    const Color roofDark = rgba(36, 46, 78);

    g.useGround(0.0f);
    g.glow(cx + 8.0f, cy + 8.0f, 50.0f, 44.0f, rgba(0, 0, 0, 130));
    g.fillRect(cx - 26.0f, cy - 22.0f, 52.0f, 38.0f, rgba(120, 112, 100));

    extrude(g, rectPts(cx - 25.0f, cy - 26.0f, cx + 25.0f, cy - 19.0f), 0.0f, 28.0f, wall, wallTop);
    for (int i = 0; i < 5; i++) {
        float mx = cx - 20.0f + i * 10.0f;
        extrude(g, rectPts(mx - 2.5f, cy - 22.5f, mx + 2.5f, cy - 19.0f), 28.0f, 33.0f, wall, wallTop);
    }
    extrude(g, rectPts(cx - 32.0f, cy - 19.0f, cx - 25.0f, cy + 14.0f), 0.0f, 24.0f, wall, wallTop);
    extrude(g, rectPts(cx + 25.0f, cy - 19.0f, cx + 32.0f, cy + 14.0f), 0.0f, 24.0f, wall, wallTop);
    castleTower(g, cx - 28.0f, cy - 22.0f, 8.5f, 38.0f, wall, roof, roofDark);
    castleTower(g, cx + 28.0f, cy - 22.0f, 8.5f, 38.0f, wall, roof, roofDark);

    extrude(g, rectPts(cx - 14.0f, cy - 16.0f, cx + 14.0f, cy + 3.0f), 0.0f, 54.0f, keepCol, shade(keepCol, 1.15f));
    for (int i = 0; i < 4; i++) {
        float mx = cx - 10.5f + i * 7.0f;
        extrude(g, rectPts(mx - 2.2f, cy - 0.5f, mx + 2.2f, cy + 3.0f), 54.0f, 60.0f, keepCol, shade(keepCol, 1.25f));
    }
    for (int side = -1; side <= 1; side += 2) {
        float mx = cx + side * 11.5f;
        extrude(g, rectPts(mx - 2.5f, cy - 16.0f, mx + 2.5f, cy - 12.0f), 54.0f, 60.0f, keepCol, shade(keepCol, 1.25f));
    }
    for (int side = -1; side <= 1; side += 2) {
        SDL_FPoint w0 = g.project(cx + side * 6.0f - 2.5f, cy + 3.0f, 30.0f);
        SDL_FPoint w1 = g.project(cx + side * 6.0f + 2.5f, cy + 3.0f, 40.0f);
        g.fillRoundRect(w0.x, w1.y, w1.x - w0.x, w0.y - w1.y, 2.5f, rgba(34, 28, 36));
        g.fillRect(w0.x, w0.y - 1.5f, w1.x - w0.x, 1.5f, rgba(255, 210, 120, 120));
    }
    for (int k = 1; k < 6; k++) {
        SDL_FPoint l0 = g.project(cx - 14.0f, cy + 3.0f, k * 9.0f);
        SDL_FPoint l1 = g.project(cx + 14.0f, cy + 3.0f, k * 9.0f);
        g.line(l0.x, l0.y, l1.x, l1.y, 0.8f, withAlpha(pal::stoneDark, 90));
    }

    SDL_FPoint pole = g.project(cx, cy - 7.0f, 54.0f);
    float poleTop = pole.y - 26.0f;
    g.line(pole.x, pole.y, pole.x, poleTop, 1.6f, rgba(70, 60, 50));
    std::vector<SDL_FPoint> flag;
    const int fs = 8;
    for (int i = 0; i <= fs; i++) {
        float fx = pole.x + 1.0f + i * 2.4f;
        float wave = std::sin(g.time() * 5.0f - i * 0.7f) * 1.6f * (i / static_cast<float>(fs));
        flag.push_back({fx, poleTop + wave});
    }
    for (int i = fs; i >= 0; i--) {
        float fx = pole.x + 1.0f + i * 2.4f;
        float wave = std::sin(g.time() * 5.0f - i * 0.7f) * 1.6f * (i / static_cast<float>(fs));
        flag.push_back({fx, poleTop + 10.0f + wave - i * 0.35f});
    }
    for (size_t i = 0; i + 1 < flag.size() / 2; i++) {
        size_t j = flag.size() - 1 - i;
        std::vector<SDL_FPoint> q = {flag[i], flag[i + 1], flag[j - 1], flag[j]};
        g.fillPolygon(q, i % 2 == 0 ? pal::gold : shade(pal::gold, 0.92f));
    }

    extrude(g, rectPts(cx - 25.0f, cy + 12.0f, cx + 25.0f, cy + 19.0f), 0.0f, 26.0f, wall, wallTop);
    for (int k = 1; k < 4; k++) {
        SDL_FPoint l0 = g.project(cx - 25.0f, cy + 19.0f, k * 6.5f);
        SDL_FPoint l1 = g.project(cx + 25.0f, cy + 19.0f, k * 6.5f);
        g.line(l0.x, l0.y, l1.x, l1.y, 0.8f, withAlpha(pal::stoneDark, 110));
    }
    for (int i = 0; i < 6; i++) {
        float mx = cx - 21.0f + i * 8.4f;
        extrude(g, rectPts(mx - 2.4f, cy + 15.5f, mx + 2.4f, cy + 19.0f), 26.0f, 31.0f, wall, wallTop);
    }
    std::vector<SDL_FPoint> gate;
    gate.push_back(g.project(cx - 8.0f, cy + 19.0f, 0.0f));
    for (int i = 0; i <= 8; i++) {
        float a = PI_F - PI_F * i / 8.0f;
        gate.push_back(g.project(cx + std::cos(a) * 8.0f, cy + 19.0f, 12.0f + std::sin(a) * 8.0f));
    }
    gate.push_back(g.project(cx + 8.0f, cy + 19.0f, 0.0f));
    g.fillPolygon(gate, rgba(28, 22, 22));
    for (int i = 0; i < 4; i++) {
        SDL_FPoint b0 = g.project(cx - 6.0f + i * 4.0f, cy + 19.0f, 0.0f);
        SDL_FPoint b1 = g.project(cx - 6.0f + i * 4.0f, cy + 19.0f, 16.0f);
        g.line(b0.x, b0.y, b1.x, b1.y, 1.0f, rgba(110, 106, 100));
    }
    SDL_FPoint h0 = g.project(cx - 8.0f, cy + 19.0f, 9.0f);
    SDL_FPoint h1 = g.project(cx + 8.0f, cy + 19.0f, 9.0f);
    g.line(h0.x, h0.y, h1.x, h1.y, 1.0f, rgba(110, 106, 100));

    castleTower(g, cx - 28.0f, cy + 16.0f, 9.5f, 36.0f, wall, roof, roofDark);
    castleTower(g, cx + 28.0f, cy + 16.0f, 9.5f, 36.0f, wall, roof, roofDark);

    if (hitFlash > 0.0f) {
        SDL_FPoint mid = g.project(cx, cy, 24.0f);
        g.glow(mid.x, mid.y, 58.0f, 48.0f, rgba(255, 60, 40, static_cast<int>(170 * std::min(1.0f, hitFlash / 0.3f))));
    }
    g.useScreen();
}

void MapView::drawAmbient(Gfx& g) const {
    const Theme& th = themeFor(theme);
    if (th.ambient == Ambient::None) return;
    const float W = PANEL_X;
    const float H = MAP_H * CELL_SIZE;
    const float t = g.time();
    int count = th.ambient == Ambient::Snow ? 90 : (th.ambient == Ambient::Embers ? 46 : 30);
    for (int i = 0; i < count; i++) {
        float a = hash01(i, 1, 503u);
        float b = hash01(i, 2, 503u);
        float c = hash01(i, 3, 503u);
        float x = 0.0f;
        float y = 0.0f;
        switch (th.ambient) {
        case Ambient::Snow: {
            float fall = 22.0f + c * 30.0f;
            y = std::fmod(b * H + t * fall, H + 20.0f) - 10.0f;
            x = std::fmod(a * W + std::sin(t * 0.8f + i) * 14.0f + t * 8.0f + W, W);
            float r = 1.0f + c * 1.8f;
            g.fillCircle(x, y, r, rgba(255, 255, 255, static_cast<int>(150 + 90 * c)));
            break;
        }
        case Ambient::Dust: {
            float drift = 30.0f + c * 40.0f;
            x = std::fmod(a * W + t * drift, W + 40.0f) - 20.0f;
            y = b * H + std::sin(t * 1.3f + i) * 8.0f;
            g.glow(x, y, 10.0f + c * 14.0f, 3.0f + c * 3.0f, rgba(238, 210, 160, static_cast<int>(30 + 30 * c)));
            break;
        }
        case Ambient::Leaves: {
            float fall = 18.0f + c * 18.0f;
            y = std::fmod(b * H + t * fall, H + 20.0f) - 10.0f;
            x = std::fmod(a * W + std::sin(t * 1.1f + i * 1.7f) * 22.0f + t * 12.0f + W, W);
            const Color cols[3] = {rgba(214, 100, 40), rgba(232, 172, 52), rgba(168, 48, 30)};
            float ang = t * (1.5f + c) + i;
            float dx = std::cos(ang) * 3.5f;
            float dy = std::sin(ang) * 1.6f;
            g.fillTriangle({x - dx, y - dy}, {x + dx, y + dy}, {x + dy, y - dx * 0.4f}, cols[i % 3]);
            break;
        }
        case Ambient::Embers: {
            float rise = 16.0f + c * 26.0f;
            y = H - std::fmod(b * H + t * rise, H + 20.0f) + 10.0f;
            x = a * W + std::sin(t * 1.7f + i) * 10.0f;
            float flick = 0.5f + 0.5f * std::sin(t * 6.0f + i);
            g.glow(x, y, 5.0f, 5.0f, rgba(255, 110, 30, static_cast<int>(60 + 60 * flick)));
            g.fillCircle(x, y, 1.2f + c, rgba(255, 200, 90, static_cast<int>(160 + 80 * flick)));
            break;
        }
        case Ambient::None:
            break;
        }
    }
}

void MapView::drawVignette(Gfx& g) const {
    const float W = PANEL_X;
    const float H = MAP_H * CELL_SIZE;
    const Color tint = themeFor(theme).vignette;
    const Color dark = tint;
    const Color clear = withAlpha(tint, 0);
    const float e = 70.0f;
    g.fillPolygonColors({{0, 0}, {W, 0}, {W, e}, {0, e}}, {dark, dark, clear, clear});
    g.fillPolygonColors({{0, H - e}, {W, H - e}, {W, H}, {0, H}}, {clear, clear, dark, dark});
    g.fillPolygonColors({{0, 0}, {e, 0}, {e, H}, {0, H}}, {dark, clear, clear, dark});
    g.fillPolygonColors({{W - e * 0.6f, 0}, {W, 0}, {W, H}, {W - e * 0.6f, H}}, {clear, dark, dark, clear});
}
