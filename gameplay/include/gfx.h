#ifndef GFX_H
#define GFX_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <string>
#include <unordered_map>
#include <vector>

struct Color {
    Uint8 r, g, b, a;
};

Color rgba(int r, int g, int b, int a = 255);
Color withAlpha(Color c, int a);
Color mix(Color a, Color b, float t);
Color shade(Color c, float factor);

namespace pal {
const Color grassDark   = {40, 64, 40, 255};
const Color grass       = {60, 92, 52, 255};
const Color grassLight  = {88, 120, 64, 255};
const Color dirt        = {156, 124, 82, 255};
const Color dirtDark    = {110, 84, 56, 255};
const Color stone       = {126, 130, 138, 255};
const Color stoneDark   = {74, 78, 88, 255};
const Color stoneLight  = {176, 178, 182, 255};
const Color wood        = {132, 90, 54, 255};
const Color woodDark    = {84, 56, 34, 255};
const Color iron        = {58, 62, 72, 255};
const Color uiBg        = {16, 19, 28, 255};
const Color uiPanel     = {25, 30, 43, 255};
const Color uiPanel2    = {35, 41, 58, 255};
const Color uiLine      = {60, 68, 92, 255};
const Color uiText      = {236, 231, 216, 255};
const Color uiMuted     = {146, 152, 170, 255};
const Color gold        = {234, 186, 78, 255};
const Color goldDeep    = {170, 122, 40, 255};
const Color danger      = {220, 76, 66, 255};
const Color good        = {104, 196, 116, 255};
const Color supply      = {96, 170, 230, 255};
const Color shadow      = {0, 0, 0, 255};
}

enum class FontId { Title, HeadingLarge, Heading, HeadingSmall, Number, BodyBold, BodyMedium, Body, Small, SmallBold, Count };
enum class Align { Left, Center, Right };

class Gfx {
public:
    Gfx(SDL_Window* window, SDL_Renderer* renderer);
    ~Gfx();
    Gfx(const Gfx&) = delete;
    Gfx& operator=(const Gfx&) = delete;

    bool loadFonts();
    bool beginFrame(float timeSeconds);
    void endFrame();

    SDL_Renderer* sdl() const { return renderer; }
    float scale() const { return pixelScale; }
    float time() const { return now; }
    static std::string assetPath(const std::string& relative);

    void setOffset(float x, float y) { offX = x; offY = y; }
    void setAlpha(float a) { alphaMul = a; }
    float alpha() const { return alphaMul; }
    void setClip(float x, float y, float w, float h);
    void clearClip();

    void clear(Color c);
    void fillRect(float x, float y, float w, float h, Color c);
    void fillRectGradient(float x, float y, float w, float h, Color top, Color bottom);
    void fillRoundRect(float x, float y, float w, float h, float r, Color c);
    void fillRoundRectGradient(float x, float y, float w, float h, float r, Color top, Color bottom);
    void strokeRoundRect(float x, float y, float w, float h, float r, float thickness, Color c);
    void fillCircle(float cx, float cy, float r, Color c);
    void fillCircleGradient(float cx, float cy, float r, Color inner, Color outer);
    void fillEllipse(float cx, float cy, float rx, float ry, Color c);
    void glow(float cx, float cy, float rx, float ry, Color c);
    void strokeCircle(float cx, float cy, float r, float thickness, Color c);
    void strokeArc(float cx, float cy, float r, float a0, float a1, float thickness, Color c);
    void line(float x1, float y1, float x2, float y2, float thickness, Color c);
    void fillPolygon(const std::vector<SDL_FPoint>& pts, Color c);
    void fillPolygonGradient(const std::vector<SDL_FPoint>& pts, Color top, Color bottom);
    void fillPolygonColors(const std::vector<SDL_FPoint>& pts, const std::vector<Color>& cols);
    void fillTriangle(SDL_FPoint a, SDL_FPoint b, SDL_FPoint c, Color col);
    void drawTexture(SDL_Texture* tex, float x, float y, float w, float h);

    void text(FontId font, const std::string& s, float x, float y, Color c, Align align = Align::Left, bool shadow = false);
    void textCentered(FontId font, const std::string& s, float cx, float cy, Color c, bool shadow = false);
    float textWidth(FontId font, const std::string& s);
    float lineHeight(FontId font);

private:
    struct CachedText {
        SDL_Texture* tex;
        int w;
        int h;
        Uint32 lastUsed;
    };

    SDL_Renderer* renderer;
    TTF_Font* fonts[static_cast<int>(FontId::Count)];
    std::unordered_map<std::string, CachedText> textCache;
    std::vector<SDL_Vertex> verts;
    std::vector<int> indices;
    std::vector<SDL_FPoint> scratch;
    float pixelScale;
    float fontScale;
    float now;
    float offX;
    float offY;
    float alphaMul;
    Uint32 frameNo;

    void closeFonts();
    SDL_Color toSdl(Color c) const;
    SDL_Vertex vert(float x, float y, Color c) const;
    void flush();
    void convex(const std::vector<SDL_FPoint>& pts, const std::vector<Color>& cols, Color centre);
    void roundRectPoints(float x, float y, float w, float h, float r, std::vector<SDL_FPoint>& out) const;
    void ellipsePoints(float cx, float cy, float rx, float ry, std::vector<SDL_FPoint>& out) const;
    int segmentsFor(float r) const;
    CachedText* cachedText(FontId font, const std::string& s, Color c);
    void purgeCache(bool all);
};

#endif
