#include "gfx.h"
#include "game_config.h"
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace {

const float PI_F = 3.14159265358979f;

struct FontSpec {
    const char* file;
    int size;
    int style;
};

const FontSpec FONT_SPECS[static_cast<int>(FontId::Count)] = {
    {"assets/fonts/CinzelDecorative-Bold.ttf", 54, TTF_STYLE_NORMAL},
    {"assets/fonts/Cinzel.ttf", 38, TTF_STYLE_BOLD},
    {"assets/fonts/Cinzel.ttf", 21, TTF_STYLE_BOLD},
    {"assets/fonts/Cinzel.ttf", 14, TTF_STYLE_BOLD},
    {"assets/fonts/Poppins-Bold.ttf", 17, TTF_STYLE_NORMAL},
    {"assets/fonts/Poppins-Bold.ttf", 13, TTF_STYLE_NORMAL},
    {"assets/fonts/Poppins-Medium.ttf", 13, TTF_STYLE_NORMAL},
    {"assets/fonts/Poppins-Regular.ttf", 13, TTF_STYLE_NORMAL},
    {"assets/fonts/Poppins-Regular.ttf", 11, TTF_STYLE_NORMAL},
    {"assets/fonts/Poppins-Bold.ttf", 11, TTF_STYLE_NORMAL},
};

bool fileExists(const std::string& path) {
    SDL_RWops* f = SDL_RWFromFile(path.c_str(), "rb");
    if (!f) return false;
    SDL_RWclose(f);
    return true;
}

}

Color rgba(int r, int g, int b, int a) {
    return Color{static_cast<Uint8>(std::clamp(r, 0, 255)), static_cast<Uint8>(std::clamp(g, 0, 255)),
                 static_cast<Uint8>(std::clamp(b, 0, 255)), static_cast<Uint8>(std::clamp(a, 0, 255))};
}

Color withAlpha(Color c, int a) {
    c.a = static_cast<Uint8>(std::clamp(a, 0, 255));
    return c;
}

Color mix(Color a, Color b, float t) {
    t = std::clamp(t, 0.0f, 1.0f);
    return rgba(static_cast<int>(a.r + (b.r - a.r) * t + 0.5f), static_cast<int>(a.g + (b.g - a.g) * t + 0.5f),
                static_cast<int>(a.b + (b.b - a.b) * t + 0.5f), static_cast<int>(a.a + (b.a - a.a) * t + 0.5f));
}

Color shade(Color c, float factor) {
    if (factor >= 1.0f) return mix(c, rgba(255, 255, 255, c.a), factor - 1.0f);
    return rgba(static_cast<int>(c.r * factor), static_cast<int>(c.g * factor), static_cast<int>(c.b * factor), c.a);
}

Gfx::Gfx(SDL_Window* w, SDL_Renderer* r)
    : renderer(r), pixelScale(1.0f), fontScale(1.0f), now(0.0f), offX(0.0f), offY(0.0f), alphaMul(1.0f),
      frameNo(0) {
    (void)w;
    for (TTF_Font*& f : fonts) f = nullptr;
    int outW = WINDOW_W;
    int outH = WINDOW_H;
    SDL_GetRendererOutputSize(renderer, &outW, &outH);
    pixelScale = std::max(1.0f, static_cast<float>(outW) / WINDOW_W);
    SDL_RenderSetScale(renderer, pixelScale, pixelScale);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
}

Gfx::~Gfx() {
    purgeCache(true);
    closeFonts();
}

std::string Gfx::assetPath(const std::string& relative) {
    if (fileExists(relative)) return relative;
    char* base = SDL_GetBasePath();
    std::string result = relative;
    if (base) {
        std::string b = base;
        SDL_free(base);
        if (fileExists(b + relative)) result = b + relative;
        else if (fileExists(b + "../" + relative)) result = b + "../" + relative;
        else if (fileExists(b + "../Resources/" + relative)) result = b + "../Resources/" + relative;
    }
    return result;
}

void Gfx::closeFonts() {
    for (TTF_Font*& f : fonts) {
        if (f) TTF_CloseFont(f);
        f = nullptr;
    }
}

bool Gfx::loadFonts() {
    closeFonts();
    fontScale = pixelScale;
    bool ok = true;
    for (int i = 0; i < static_cast<int>(FontId::Count); i++) {
        std::string path = assetPath(FONT_SPECS[i].file);
        int size = static_cast<int>(std::lround(FONT_SPECS[i].size * fontScale));
        fonts[i] = TTF_OpenFont(path.c_str(), size);
        if (!fonts[i]) {
            std::fprintf(stderr, "Font load failed: %s (%s)\n", path.c_str(), TTF_GetError());
            ok = false;
            continue;
        }
        TTF_SetFontStyle(fonts[i], FONT_SPECS[i].style);
        TTF_SetFontHinting(fonts[i], TTF_HINTING_LIGHT);
    }
    return ok;
}

bool Gfx::beginFrame(float timeSeconds) {
    now = timeSeconds;
    int outW = WINDOW_W;
    int outH = WINDOW_H;
    SDL_GetRendererOutputSize(renderer, &outW, &outH);
    float s = std::max(1.0f, static_cast<float>(outW) / WINDOW_W);
    bool changed = std::fabs(s - pixelScale) > 0.01f;
    if (changed) {
        pixelScale = s;
        SDL_RenderSetScale(renderer, pixelScale, pixelScale);
        purgeCache(true);
        loadFonts();
    }
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    offX = 0.0f;
    offY = 0.0f;
    alphaMul = 1.0f;
    return changed;
}

void Gfx::endFrame() {
    SDL_RenderPresent(renderer);
    frameNo++;
    if (frameNo % 600 == 0) purgeCache(false);
}

void Gfx::setClip(float x, float y, float w, float h) {
    SDL_Rect r = {static_cast<int>(x), static_cast<int>(y), static_cast<int>(w), static_cast<int>(h)};
    SDL_RenderSetClipRect(renderer, &r);
}

void Gfx::clearClip() {
    SDL_RenderSetClipRect(renderer, nullptr);
}

SDL_Color Gfx::toSdl(Color c) const {
    SDL_Color s;
    s.r = c.r;
    s.g = c.g;
    s.b = c.b;
    s.a = static_cast<Uint8>(std::clamp(c.a * alphaMul, 0.0f, 255.0f));
    return s;
}

SDL_Vertex Gfx::vert(float x, float y, Color c) const {
    SDL_Vertex v;
    v.position.x = x + offX;
    v.position.y = y + offY;
    v.color = toSdl(c);
    v.tex_coord.x = 0.0f;
    v.tex_coord.y = 0.0f;
    return v;
}

void Gfx::flush() {
    if (!verts.empty() && !indices.empty()) {
        SDL_RenderGeometry(renderer, nullptr, verts.data(), static_cast<int>(verts.size()), indices.data(),
                           static_cast<int>(indices.size()));
    }
    verts.clear();
    indices.clear();
}

void Gfx::clear(Color c) {
    SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, 255);
    SDL_RenderClear(renderer);
}

int Gfx::segmentsFor(float r) const {
    return std::clamp(static_cast<int>(r * pixelScale * 0.9f) + 10, 14, 96);
}

void Gfx::convex(const std::vector<SDL_FPoint>& pts, const std::vector<Color>& cols, Color centre) {
    const int n = static_cast<int>(pts.size());
    if (n < 3) return;
    float area = 0.0f;
    float cx = 0.0f;
    float cy = 0.0f;
    for (int i = 0; i < n; i++) {
        const SDL_FPoint& a = pts[i];
        const SDL_FPoint& b = pts[(i + 1) % n];
        area += a.x * b.y - b.x * a.y;
        cx += a.x;
        cy += a.y;
    }
    cx /= n;
    cy /= n;
    const float sign = area >= 0.0f ? 1.0f : -1.0f;
    const float feather = 1.0f / pixelScale;

    verts.push_back(vert(cx, cy, centre));
    for (int i = 0; i < n; i++) {
        const SDL_FPoint& p = pts[i];
        const SDL_FPoint& prev = pts[(i + n - 1) % n];
        const SDL_FPoint& next = pts[(i + 1) % n];
        float d0x = p.x - prev.x;
        float d0y = p.y - prev.y;
        float d1x = next.x - p.x;
        float d1y = next.y - p.y;
        float l0 = std::sqrt(d0x * d0x + d0y * d0y);
        float l1 = std::sqrt(d1x * d1x + d1y * d1y);
        if (l0 < 1e-5f) l0 = 1.0f;
        if (l1 < 1e-5f) l1 = 1.0f;
        float n0x = sign * d0y / l0;
        float n0y = -sign * d0x / l0;
        float n1x = sign * d1y / l1;
        float n1y = -sign * d1x / l1;
        float nx = n0x + n1x;
        float ny = n0y + n1y;
        float nl = std::sqrt(nx * nx + ny * ny);
        if (nl < 1e-5f) {
            nx = n0x;
            ny = n0y;
        } else {
            nx /= nl;
            ny /= nl;
        }
        float dot = std::max(0.35f, nx * n0x + ny * n0y);
        float m = 0.5f * feather / dot;
        Color c = cols[i];
        verts.push_back(vert(p.x - nx * m, p.y - ny * m, c));
        verts.push_back(vert(p.x + nx * m, p.y + ny * m, withAlpha(c, 0)));
    }
    for (int i = 0; i < n; i++) {
        int j = (i + 1) % n;
        int in0 = 1 + i * 2;
        int out0 = in0 + 1;
        int in1 = 1 + j * 2;
        int out1 = in1 + 1;
        indices.push_back(0);
        indices.push_back(in0);
        indices.push_back(in1);
        indices.push_back(in0);
        indices.push_back(out0);
        indices.push_back(out1);
        indices.push_back(in0);
        indices.push_back(out1);
        indices.push_back(in1);
    }
    flush();
}

void Gfx::fillPolygon(const std::vector<SDL_FPoint>& pts, Color c) {
    std::vector<Color> cols(pts.size(), c);
    convex(pts, cols, c);
}

void Gfx::fillPolygonGradient(const std::vector<SDL_FPoint>& pts, Color top, Color bottom) {
    if (pts.empty()) return;
    float minY = pts[0].y;
    float maxY = pts[0].y;
    float sumY = 0.0f;
    for (const SDL_FPoint& p : pts) {
        minY = std::min(minY, p.y);
        maxY = std::max(maxY, p.y);
        sumY += p.y;
    }
    float h = std::max(1e-3f, maxY - minY);
    std::vector<Color> cols;
    cols.reserve(pts.size());
    for (const SDL_FPoint& p : pts) cols.push_back(mix(top, bottom, (p.y - minY) / h));
    convex(pts, cols, mix(top, bottom, (sumY / pts.size() - minY) / h));
}

void Gfx::fillPolygonColors(const std::vector<SDL_FPoint>& pts, const std::vector<Color>& cols) {
    if (pts.size() < 3 || cols.size() != pts.size()) return;
    int r = 0, gg = 0, b = 0, a = 0;
    for (const Color& c : cols) {
        r += c.r;
        gg += c.g;
        b += c.b;
        a += c.a;
    }
    int n = static_cast<int>(cols.size());
    convex(pts, cols, rgba(r / n, gg / n, b / n, a / n));
}

void Gfx::fillTriangle(SDL_FPoint a, SDL_FPoint b, SDL_FPoint c, Color col) {
    scratch.assign({a, b, c});
    fillPolygon(scratch, col);
}

void Gfx::fillRect(float x, float y, float w, float h, Color c) {
    scratch.assign({{x, y}, {x + w, y}, {x + w, y + h}, {x, y + h}});
    fillPolygon(scratch, c);
}

void Gfx::fillRectGradient(float x, float y, float w, float h, Color top, Color bottom) {
    scratch.assign({{x, y}, {x + w, y}, {x + w, y + h}, {x, y + h}});
    fillPolygonGradient(scratch, top, bottom);
}

void Gfx::roundRectPoints(float x, float y, float w, float h, float r, std::vector<SDL_FPoint>& out) const {
    out.clear();
    r = std::min(r, std::min(w, h) * 0.5f);
    if (r < 0.5f) {
        out.assign({{x, y}, {x + w, y}, {x + w, y + h}, {x, y + h}});
        return;
    }
    int seg = std::max(3, segmentsFor(r) / 4);
    const float cxs[4] = {x + w - r, x + w - r, x + r, x + r};
    const float cys[4] = {y + r, y + h - r, y + h - r, y + r};
    const float start[4] = {-PI_F * 0.5f, 0.0f, PI_F * 0.5f, PI_F};
    for (int c = 0; c < 4; c++) {
        for (int i = 0; i <= seg; i++) {
            float a = start[c] + (PI_F * 0.5f) * i / seg;
            out.push_back({cxs[c] + std::cos(a) * r, cys[c] + std::sin(a) * r});
        }
    }
}

void Gfx::ellipsePoints(float cx, float cy, float rx, float ry, std::vector<SDL_FPoint>& out) const {
    out.clear();
    int seg = segmentsFor(std::max(rx, ry));
    for (int i = 0; i < seg; i++) {
        float a = 2.0f * PI_F * i / seg;
        out.push_back({cx + std::cos(a) * rx, cy + std::sin(a) * ry});
    }
}

void Gfx::fillRoundRect(float x, float y, float w, float h, float r, Color c) {
    roundRectPoints(x, y, w, h, r, scratch);
    fillPolygon(scratch, c);
}

void Gfx::fillRoundRectGradient(float x, float y, float w, float h, float r, Color top, Color bottom) {
    roundRectPoints(x, y, w, h, r, scratch);
    fillPolygonGradient(scratch, top, bottom);
}

void Gfx::strokeRoundRect(float x, float y, float w, float h, float r, float t, Color c) {
    std::vector<SDL_FPoint> outer;
    std::vector<SDL_FPoint> inner;
    roundRectPoints(x - t * 0.5f, y - t * 0.5f, w + t, h + t, r + t * 0.5f, outer);
    roundRectPoints(x + t * 0.5f, y + t * 0.5f, w - t, h - t, std::max(0.0f, r - t * 0.5f), inner);
    if (outer.size() != inner.size()) {
        line(x, y, x + w, y, t, c);
        line(x + w, y, x + w, y + h, t, c);
        line(x + w, y + h, x, y + h, t, c);
        line(x, y + h, x, y, t, c);
        return;
    }
    const int n = static_cast<int>(outer.size());
    const float f = 0.5f / pixelScale;
    for (int i = 0; i < n; i++) {
        const SDL_FPoint& o = outer[i];
        const SDL_FPoint& in = inner[i];
        float dx = o.x - in.x;
        float dy = o.y - in.y;
        float l = std::sqrt(dx * dx + dy * dy);
        if (l < 1e-5f) l = 1.0f;
        dx /= l;
        dy /= l;
        verts.push_back(vert(in.x - dx * f, in.y - dy * f, withAlpha(c, 0)));
        verts.push_back(vert(in.x + dx * f, in.y + dy * f, c));
        verts.push_back(vert(o.x - dx * f, o.y - dy * f, c));
        verts.push_back(vert(o.x + dx * f, o.y + dy * f, withAlpha(c, 0)));
    }
    for (int i = 0; i < n; i++) {
        int a = i * 4;
        int b = ((i + 1) % n) * 4;
        for (int k = 0; k < 3; k++) {
            indices.push_back(a + k);
            indices.push_back(a + k + 1);
            indices.push_back(b + k + 1);
            indices.push_back(a + k);
            indices.push_back(b + k + 1);
            indices.push_back(b + k);
        }
    }
    flush();
}

void Gfx::fillCircle(float cx, float cy, float r, Color c) {
    ellipsePoints(cx, cy, r, r, scratch);
    fillPolygon(scratch, c);
}

void Gfx::fillEllipse(float cx, float cy, float rx, float ry, Color c) {
    ellipsePoints(cx, cy, rx, ry, scratch);
    fillPolygon(scratch, c);
}

void Gfx::fillCircleGradient(float cx, float cy, float r, Color inner, Color outer) {
    ellipsePoints(cx, cy, r, r, scratch);
    std::vector<Color> cols(scratch.size(), outer);
    convex(scratch, cols, inner);
}

void Gfx::glow(float cx, float cy, float rx, float ry, Color c) {
    ellipsePoints(cx, cy, rx, ry, scratch);
    std::vector<Color> cols(scratch.size(), withAlpha(c, 0));
    convex(scratch, cols, c);
}

void Gfx::strokeArc(float cx, float cy, float r, float a0, float a1, float t, Color c) {
    float span = a1 - a0;
    int seg = std::max(4, static_cast<int>(segmentsFor(r) * std::fabs(span) / (2.0f * PI_F)) + 1);
    const float f = 0.5f / pixelScale;
    const float half = t * 0.5f;
    const float radii[4] = {r - half - f, r - half + f, r + half - f, r + half + f};
    for (int i = 0; i <= seg; i++) {
        float a = a0 + span * i / seg;
        float ca = std::cos(a);
        float sa = std::sin(a);
        for (int k = 0; k < 4; k++) {
            float rr = std::max(0.0f, radii[k]);
            Color col = (k == 0 || k == 3) ? withAlpha(c, 0) : c;
            verts.push_back(vert(cx + ca * rr, cy + sa * rr, col));
        }
    }
    for (int i = 0; i < seg; i++) {
        int a = i * 4;
        int b = (i + 1) * 4;
        for (int k = 0; k < 3; k++) {
            indices.push_back(a + k);
            indices.push_back(a + k + 1);
            indices.push_back(b + k + 1);
            indices.push_back(a + k);
            indices.push_back(b + k + 1);
            indices.push_back(b + k);
        }
    }
    flush();
}

void Gfx::strokeCircle(float cx, float cy, float r, float t, Color c) {
    strokeArc(cx, cy, r, 0.0f, 2.0f * PI_F, t, c);
}

void Gfx::line(float x1, float y1, float x2, float y2, float t, Color c) {
    float dx = x2 - x1;
    float dy = y2 - y1;
    float l = std::sqrt(dx * dx + dy * dy);
    if (l < 1e-4f) return;
    float nx = -dy / l * t * 0.5f;
    float ny = dx / l * t * 0.5f;
    scratch.assign({{x1 + nx, y1 + ny}, {x2 + nx, y2 + ny}, {x2 - nx, y2 - ny}, {x1 - nx, y1 - ny}});
    fillPolygon(scratch, c);
}

void Gfx::drawTexture(SDL_Texture* tex, float x, float y, float w, float h) {
    if (!tex) return;
    SDL_FRect dst = {x + offX, y + offY, w, h};
    SDL_SetTextureAlphaMod(tex, static_cast<Uint8>(std::clamp(255.0f * alphaMul, 0.0f, 255.0f)));
    SDL_RenderCopyF(renderer, tex, nullptr, &dst);
}

Gfx::CachedText* Gfx::cachedText(FontId font, const std::string& s, Color c) {
    TTF_Font* f = fonts[static_cast<int>(font)];
    if (!f || s.empty()) return nullptr;
    char head[32];
    std::snprintf(head, sizeof(head), "%d|%02x%02x%02x|", static_cast<int>(font), c.r, c.g, c.b);
    std::string key = head + s;
    auto it = textCache.find(key);
    if (it != textCache.end()) {
        it->second.lastUsed = frameNo;
        return &it->second;
    }
    SDL_Color col = {c.r, c.g, c.b, 255};
    SDL_Surface* surf = TTF_RenderUTF8_Blended(f, s.c_str(), col);
    if (!surf) return nullptr;
    SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surf);
    CachedText entry = {tex, surf->w, surf->h, frameNo};
    SDL_FreeSurface(surf);
    if (!tex) return nullptr;
    SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
    auto res = textCache.emplace(key, entry);
    return &res.first->second;
}

void Gfx::purgeCache(bool all) {
    for (auto it = textCache.begin(); it != textCache.end();) {
        if (all || frameNo - it->second.lastUsed > 600) {
            SDL_DestroyTexture(it->second.tex);
            it = textCache.erase(it);
        } else {
            ++it;
        }
    }
}

float Gfx::textWidth(FontId font, const std::string& s) {
    TTF_Font* f = fonts[static_cast<int>(font)];
    if (!f || s.empty()) return 0.0f;
    int w = 0;
    int h = 0;
    TTF_SizeUTF8(f, s.c_str(), &w, &h);
    return w / fontScale;
}

float Gfx::lineHeight(FontId font) {
    TTF_Font* f = fonts[static_cast<int>(font)];
    if (!f) return 14.0f;
    return TTF_FontHeight(f) / fontScale;
}

void Gfx::text(FontId font, const std::string& s, float x, float y, Color c, Align align, bool shadow) {
    if (shadow) {
        CachedText* sh = cachedText(font, s, rgba(0, 0, 0));
        if (sh) {
            float sw = sh->w / fontScale;
            float shh = sh->h / fontScale;
            float sx = x;
            if (align == Align::Center) sx -= sw * 0.5f;
            else if (align == Align::Right) sx -= sw;
            SDL_FRect d = {sx + offX + 1.0f, y + offY + 1.5f, sw, shh};
            SDL_SetTextureAlphaMod(sh->tex, static_cast<Uint8>(std::clamp(c.a * alphaMul * 0.6f, 0.0f, 255.0f)));
            SDL_RenderCopyF(renderer, sh->tex, nullptr, &d);
        }
    }
    CachedText* t = cachedText(font, s, c);
    if (!t) return;
    float w = t->w / fontScale;
    float h = t->h / fontScale;
    if (align == Align::Center) x -= w * 0.5f;
    else if (align == Align::Right) x -= w;
    SDL_FRect dst = {x + offX, y + offY, w, h};
    SDL_SetTextureAlphaMod(t->tex, static_cast<Uint8>(std::clamp(c.a * alphaMul, 0.0f, 255.0f)));
    SDL_RenderCopyF(renderer, t->tex, nullptr, &dst);
}

void Gfx::textCentered(FontId font, const std::string& s, float cx, float cy, Color c, bool shadow) {
    text(font, s, cx, cy - lineHeight(font) * 0.5f, c, Align::Center, shadow);
}
