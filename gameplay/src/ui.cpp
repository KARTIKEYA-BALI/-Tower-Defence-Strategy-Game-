#include "ui.h"
#include <algorithm>
#include <cmath>
#include <sstream>

Button::Button() : rect{0, 0, 0, 0}, style(ButtonStyle::Secondary), enabled(true) {
}

Button::Button(SDL_FRect r, const std::string& l, ButtonStyle s) : rect(r), label(l), style(s), enabled(true) {
}

bool Button::contains(float mx, float my) const {
    return mx >= rect.x && mx < rect.x + rect.w && my >= rect.y && my < rect.y + rect.h;
}

void Button::draw(Gfx& g, const MouseState& mouse, const std::string& subLabel) const {
    bool hover = enabled && contains(mouse.x, mouse.y);
    bool pressed = hover && mouse.down;
    float y = rect.y + (pressed ? 1.0f : 0.0f);
    const float r = 8.0f;

    Color top;
    Color bottom;
    Color border;
    Color text;
    if (!enabled) {
        top = rgba(40, 45, 60);
        bottom = rgba(33, 38, 52);
        border = rgba(52, 58, 76);
        text = pal::uiMuted;
    } else if (style == ButtonStyle::Primary) {
        top = rgba(246, 206, 112);
        bottom = rgba(200, 144, 50);
        border = rgba(120, 82, 24);
        text = rgba(46, 30, 8);
    } else if (style == ButtonStyle::Danger) {
        top = rgba(196, 80, 70);
        bottom = rgba(146, 46, 42);
        border = rgba(90, 24, 22);
        text = rgba(255, 240, 232);
    } else {
        top = rgba(58, 66, 90);
        bottom = rgba(42, 48, 68);
        border = rgba(76, 86, 116);
        text = pal::uiText;
    }
    if (hover) {
        top = shade(top, 1.12f);
        bottom = shade(bottom, 1.1f);
        if (style == ButtonStyle::Secondary) border = withAlpha(pal::gold, 200);
    }
    if (pressed) {
        top = shade(top, 0.9f);
        bottom = shade(bottom, 0.9f);
    }

    if (enabled && !pressed) g.glow(rect.x + rect.w * 0.5f, rect.y + rect.h + 1.0f, rect.w * 0.55f, 7.0f, rgba(0, 0, 0, 110));
    g.fillRoundRect(rect.x - 1.0f, y - 1.0f, rect.w + 2.0f, rect.h + 2.0f, r + 1.0f, border);
    g.fillRoundRectGradient(rect.x, y, rect.w, rect.h, r, top, bottom);
    g.fillRoundRect(rect.x + 3.0f, y + 1.0f, rect.w - 6.0f, 1.2f, 0.6f, rgba(255, 255, 255, enabled ? 60 : 15));

    float cx = rect.x + rect.w * 0.5f;
    if (subLabel.empty()) {
        g.textCentered(FontId::BodyBold, label, cx, y + rect.h * 0.5f + 0.5f, text);
    } else {
        g.textCentered(FontId::BodyBold, label, cx, y + rect.h * 0.5f - 6.5f, text);
        g.textCentered(FontId::Small, subLabel, cx, y + rect.h * 0.5f + 8.5f, withAlpha(text, 200));
    }
}

TextBox::TextBox() : rect{0, 0, 0, 0} {
}

TextBox::TextBox(SDL_FRect r, const std::string& p) : rect(r), placeholder(p) {
}

bool TextBox::contains(float mx, float my) const {
    return mx >= rect.x && mx < rect.x + rect.w && my >= rect.y && my < rect.y + rect.h;
}

void TextBox::draw(Gfx& g, const std::string& text, bool focused, const MouseState& mouse) const {
    bool hover = contains(mouse.x, mouse.y);
    Color border = focused ? pal::gold : (hover ? rgba(96, 108, 140) : pal::uiLine);
    g.fillRoundRect(rect.x - 1.0f, rect.y - 1.0f, rect.w + 2.0f, rect.h + 2.0f, 8.0f, border);
    g.fillRoundRectGradient(rect.x, rect.y, rect.w, rect.h, 7.0f, rgba(14, 17, 26), rgba(22, 26, 38));
    icons::magnifier(g, rect.x + 15.0f, rect.y + rect.h * 0.5f, 10.0f, focused ? pal::gold : pal::uiMuted);
    float tx = rect.x + 28.0f;
    float ty = rect.y + rect.h * 0.5f;
    if (text.empty()) {
        g.textCentered(FontId::Body, placeholder, tx + g.textWidth(FontId::Body, placeholder) * 0.5f, ty, rgba(110, 118, 140));
    } else {
        g.textCentered(FontId::BodyMedium, text, tx + g.textWidth(FontId::BodyMedium, text) * 0.5f, ty, pal::uiText);
    }
    if (focused && std::fmod(g.time(), 1.0f) < 0.55f) {
        float cx = tx + (text.empty() ? 0.0f : g.textWidth(FontId::BodyMedium, text) + 1.5f);
        g.line(cx, ty - 7.0f, cx, ty + 7.0f, 1.4f, pal::gold);
    }
}

namespace icons {

void heart(Gfx& g, float cx, float cy, float s, Color c) {
    float r = s * 0.28f;
    g.fillCircle(cx - r * 0.95f, cy - r * 0.45f, r, c);
    g.fillCircle(cx + r * 0.95f, cy - r * 0.45f, r, c);
    g.fillTriangle({cx - r * 1.92f, cy - r * 0.2f}, {cx + r * 1.92f, cy - r * 0.2f}, {cx, cy + s * 0.5f}, c);
    g.fillCircle(cx - r * 1.1f, cy - r * 0.75f, r * 0.35f, rgba(255, 255, 255, 110));
}

void coin(Gfx& g, float cx, float cy, float r) {
    g.fillCircle(cx, cy + 0.8f, r, rgba(120, 80, 18));
    g.fillCircleGradient(cx, cy, r, rgba(255, 226, 130), rgba(214, 156, 46));
    g.strokeCircle(cx, cy, r * 0.62f, std::max(0.8f, r * 0.14f), rgba(176, 120, 30, 200));
    g.fillCircle(cx - r * 0.35f, cy - r * 0.38f, r * 0.22f, rgba(255, 255, 255, 150));
}

void flag(Gfx& g, float cx, float cy, float s, Color c) {
    float left = cx - s * 0.38f;
    g.line(left, cy - s * 0.5f, left, cy + s * 0.5f, std::max(1.2f, s * 0.11f), rgba(200, 196, 186));
    g.fillPolygon({{left + 0.5f, cy - s * 0.48f}, {cx + s * 0.45f, cy - s * 0.3f}, {left + 0.5f, cy - s * 0.04f}}, c);
}

void magnifier(Gfx& g, float cx, float cy, float s, Color c) {
    g.strokeCircle(cx - s * 0.12f, cy - s * 0.12f, s * 0.32f, std::max(1.3f, s * 0.13f), c);
    g.line(cx + s * 0.13f, cy + s * 0.13f, cx + s * 0.45f, cy + s * 0.45f, std::max(1.5f, s * 0.16f), c);
}

void pause(Gfx& g, float cx, float cy, float s, Color c) {
    g.fillRoundRect(cx - s * 0.36f, cy - s * 0.4f, s * 0.24f, s * 0.8f, 1.5f, c);
    g.fillRoundRect(cx + s * 0.12f, cy - s * 0.4f, s * 0.24f, s * 0.8f, 1.5f, c);
}

void fastForward(Gfx& g, float cx, float cy, float s, Color c) {
    g.fillTriangle({cx - s * 0.5f, cy - s * 0.38f}, {cx, cy}, {cx - s * 0.5f, cy + s * 0.38f}, c);
    g.fillTriangle({cx, cy - s * 0.38f}, {cx + s * 0.5f, cy}, {cx, cy + s * 0.38f}, c);
}

void play(Gfx& g, float cx, float cy, float s, Color c) {
    g.fillTriangle({cx - s * 0.3f, cy - s * 0.42f}, {cx + s * 0.42f, cy}, {cx - s * 0.3f, cy + s * 0.42f}, c);
}

}

std::vector<std::string> wrapText(Gfx& g, FontId font, const std::string& text, float maxWidth) {
    std::vector<std::string> lines;
    std::istringstream in(text);
    std::string word;
    std::string current;
    while (in >> word) {
        std::string candidate = current.empty() ? word : current + " " + word;
        if (!current.empty() && g.textWidth(font, candidate) > maxWidth) {
            lines.push_back(current);
            current = word;
        } else {
            current = candidate;
        }
    }
    if (!current.empty()) lines.push_back(current);
    return lines;
}

void sectionHeading(Gfx& g, const std::string& title, float x, float y, float width) {
    g.text(FontId::HeadingSmall, title, x, y, pal::gold);
    float tw = g.textWidth(FontId::HeadingSmall, title);
    float ly = y + g.lineHeight(FontId::HeadingSmall) * 0.5f + 0.5f;
    g.fillPolygonColors({{x + tw + 8.0f, ly - 0.6f}, {x + width, ly - 0.6f}, {x + width, ly + 0.6f}, {x + tw + 8.0f, ly + 0.6f}},
                        {withAlpha(pal::gold, 120), withAlpha(pal::gold, 0), withAlpha(pal::gold, 0), withAlpha(pal::gold, 120)});
    float d = 2.6f;
    float dx = x + tw + 8.0f;
    g.fillPolygon({{dx, ly - d}, {dx + d, ly}, {dx, ly + d}, {dx - d, ly}}, pal::gold);
}

void panelCard(Gfx& g, float x, float y, float w, float h) {
    g.glow(x + w * 0.5f, y + h * 0.5f + 8.0f, w * 0.62f, h * 0.62f, rgba(0, 0, 0, 160));
    g.fillRoundRect(x - 1.0f, y - 1.0f, w + 2.0f, h + 2.0f, 15.0f, rgba(120, 92, 44));
    g.fillRoundRectGradient(x, y, w, h, 14.0f, rgba(36, 42, 60), rgba(20, 24, 35));
    g.strokeRoundRect(x + 5.0f, y + 5.0f, w - 10.0f, h - 10.0f, 10.0f, 1.0f, withAlpha(pal::gold, 60));
}
