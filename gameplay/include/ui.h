#ifndef UI_H
#define UI_H

#include "gfx.h"
#include <string>
#include <vector>

enum class ButtonStyle { Primary, Secondary, Danger };

struct MouseState {
    float x;
    float y;
    bool down;
};

class Button {
public:
    Button();
    Button(SDL_FRect rect, const std::string& label, ButtonStyle style);

    bool contains(float mx, float my) const;
    void draw(Gfx& g, const MouseState& mouse, const std::string& subLabel = "") const;

    SDL_FRect rect;
    std::string label;
    ButtonStyle style;
    bool enabled;
};

class TextBox {
public:
    TextBox();
    explicit TextBox(SDL_FRect rect, const std::string& placeholder);

    bool contains(float mx, float my) const;
    void draw(Gfx& g, const std::string& text, bool focused, const MouseState& mouse) const;

    SDL_FRect rect;
    std::string placeholder;
};

namespace icons {
void heart(Gfx& g, float cx, float cy, float size, Color c);
void coin(Gfx& g, float cx, float cy, float r);
void flag(Gfx& g, float cx, float cy, float size, Color c);
void magnifier(Gfx& g, float cx, float cy, float size, Color c);
void pause(Gfx& g, float cx, float cy, float size, Color c);
void fastForward(Gfx& g, float cx, float cy, float size, Color c);
void play(Gfx& g, float cx, float cy, float size, Color c);
}

std::vector<std::string> wrapText(Gfx& g, FontId font, const std::string& text, float maxWidth);
void sectionHeading(Gfx& g, const std::string& title, float x, float y, float width);
void panelCard(Gfx& g, float x, float y, float w, float h);

#endif
