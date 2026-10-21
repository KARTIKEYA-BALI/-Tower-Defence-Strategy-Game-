#include "side_panel.h"
#include "game_config.h"
#include "tower.h"
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace {

const float PI_F = 3.14159265358979f;

std::string fmt(const char* f, double v) {
    char buf[32];
    std::snprintf(buf, sizeof(buf), f, v);
    return buf;
}

}

SidePanel::SidePanel()
    : x0(PANEL_X + 14.0f), width(PANEL_W - 28.0f), cardsTop(200.0f), cardHeight(50.0f), cardGap(6.0f), infoTop(468.0f) {
    const float half = (width - 6.0f) * 0.5f;
    waveButton = Button({x0, 122.0f, width, 44.0f}, "Start Wave", ButtonStyle::Primary);
    upgradeButton = Button({x0, 566.0f, half, 32.0f}, "Upgrade", ButtonStyle::Primary);
    sellButton = Button({x0 + half + 6.0f, 566.0f, half, 32.0f}, "Sell", ButtonStyle::Danger);
    saveButton = Button({x0, 616.0f, half, 30.0f}, "Save", ButtonStyle::Secondary);
    loadButton = Button({x0 + half + 6.0f, 616.0f, half, 30.0f}, "Load", ButtonStyle::Secondary);
    speedButton = Button({x0, 652.0f, half, 30.0f}, "Speed 1x", ButtonStyle::Secondary);
    pauseButton = Button({x0 + half + 6.0f, 652.0f, half, 30.0f}, "Pause", ButtonStyle::Secondary);
}

SDL_FRect SidePanel::cardRect(int slot) const {
    return {x0, cardsTop + slot * (cardHeight + cardGap), width, cardHeight};
}

int SidePanel::cardAt(float mx, float my, const PanelModel& m) const {
    for (int i = 0; i < static_cast<int>(m.results.size()); i++) {
        SDL_FRect r = cardRect(i);
        if (mx >= r.x && mx < r.x + r.w && my >= r.y && my < r.y + r.h) return m.results[i];
    }
    return -1;
}

PanelHit SidePanel::hitTest(float mx, float my, const PanelModel& m) const {
    if (waveButton.contains(mx, my) && !m.waveActive && !m.allWavesDone) return {PanelAction::StartWave, -1};
    int card = cardAt(mx, my, m);
    if (card >= 0) return {PanelAction::PickType, card};
    if (m.selected) {
        if (upgradeButton.contains(mx, my) && m.selected->canUpgrade()) return {PanelAction::Upgrade, -1};
        if (sellButton.contains(mx, my)) return {PanelAction::Sell, -1};
    }
    if (saveButton.contains(mx, my)) return {PanelAction::Save, -1};
    if (loadButton.contains(mx, my)) return {PanelAction::Load, -1};
    if (speedButton.contains(mx, my)) return {PanelAction::Speed, -1};
    if (pauseButton.contains(mx, my)) return {PanelAction::Pause, -1};
    return {PanelAction::None, -1};
}

void SidePanel::draw(Gfx& g, const PanelModel& m, const MouseState& mouse) const {
    g.fillRectGradient(PANEL_X, 0, PANEL_W, WINDOW_H, rgba(28, 33, 48), rgba(15, 18, 27));
    g.fillPolygonColors({{PANEL_X - 14.0f, 0}, {PANEL_X, 0}, {PANEL_X, WINDOW_H}, {PANEL_X - 14.0f, WINDOW_H}},
                        {rgba(0, 0, 0, 0), rgba(0, 0, 0, 120), rgba(0, 0, 0, 120), rgba(0, 0, 0, 0)});
    g.fillRect(PANEL_X, 0, 2.0f, WINDOW_H, rgba(110, 86, 44));
    g.fillRect(PANEL_X + 2.0f, 0, 1.0f, WINDOW_H, rgba(0, 0, 0, 140));

    drawHeader(g, m);
    drawWaveButton(g, m, mouse);

    const std::string keys = "keys 1-4";
    float keysW = g.textWidth(FontId::Small, keys);
    sectionHeading(g, "ARMORY", x0, 176.0f, width - keysW - 8.0f);
    g.text(FontId::Small, keys, x0 + width, 177.0f, pal::uiMuted, Align::Right);
    drawCards(g, m, mouse);
    drawInfo(g, m, mouse);

    Button speed = speedButton;
    speed.label = m.fastSpeed ? "Speed 2x" : "Speed 1x";
    saveButton.draw(g, mouse);
    loadButton.draw(g, mouse);
    speed.draw(g, mouse);
    pauseButton.draw(g, mouse);
}

void SidePanel::drawHeader(Gfx& g, const PanelModel& m) const {
    float y = 14.0f;
    icons::heart(g, x0 + 8.0f, y + 9.0f, 15.0f, pal::danger);
    g.text(FontId::Small, "CASTLE", x0 + 22.0f, y + 2.0f, pal::uiMuted);
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%d / %d", std::max(0, m.health), m.maxHealth);
    g.text(FontId::BodyBold, buf, x0 + width, y, pal::uiText, Align::Right);

    float barY = y + 24.0f;
    float frac = std::clamp(static_cast<float>(m.health) / static_cast<float>(m.maxHealth), 0.0f, 1.0f);
    g.fillRoundRect(x0, barY, width, 9.0f, 4.5f, rgba(10, 12, 18));
    if (frac > 0.0f) {
        Color hi = frac > 0.5f ? mix(rgba(236, 196, 70), pal::good, (frac - 0.5f) * 2.0f) : mix(pal::danger, rgba(236, 196, 70), frac * 2.0f);
        g.fillRoundRectGradient(x0 + 1.0f, barY + 1.0f, std::max(7.0f, (width - 2.0f) * frac), 7.0f, 3.5f, shade(hi, 1.15f), shade(hi, 0.8f));
    }

    float gy = 58.0f;
    icons::coin(g, x0 + 8.0f, gy + 10.0f, 7.0f);
    g.text(FontId::Small, "GOLD", x0 + 22.0f, gy + 3.0f, pal::uiMuted);
    g.text(FontId::Number, std::to_string(m.gold), x0 + width, gy - 2.0f, pal::gold, Align::Right);

    float wy = 86.0f;
    icons::flag(g, x0 + 8.0f, wy + 10.0f, 16.0f, pal::supply);
    g.text(FontId::Small, "WAVE", x0 + 22.0f, wy + 3.0f, pal::uiMuted);
    std::snprintf(buf, sizeof(buf), "%d / %d", m.wave, m.totalWaves);
    g.text(FontId::Number, buf, x0 + width, wy - 2.0f, pal::uiText, Align::Right);

    g.fillRect(x0, 114.0f, width, 1.0f, rgba(255, 255, 255, 18));
}

void SidePanel::drawWaveButton(Gfx& g, const PanelModel& m, const MouseState& mouse) const {
    Button b = waveButton;
    std::string sub;
    char buf[64];
    if (m.allWavesDone) {
        b.enabled = false;
        b.label = "All waves repelled";
        sub = "The siege is broken";
    } else if (m.waveActive) {
        b.enabled = false;
        std::snprintf(buf, sizeof(buf), "Wave %d under way", m.wave);
        b.label = buf;
        std::snprintf(buf, sizeof(buf), "%d enem%s left", m.enemiesLeft, m.enemiesLeft == 1 ? "y" : "ies");
        sub = buf;
    } else if (m.countdown > 0.0f) {
        std::snprintf(buf, sizeof(buf), "Call Wave %d", m.wave);
        b.label = buf;
        std::snprintf(buf, sizeof(buf), "in %ds  -  early bonus +%d", static_cast<int>(std::ceil(m.countdown)), m.earlyBonus);
        sub = buf;
    } else {
        std::snprintf(buf, sizeof(buf), "Start Wave %d", m.wave);
        b.label = buf;
        sub = "Space";
    }
    b.draw(g, mouse, sub);
}

void SidePanel::drawCards(Gfx& g, const PanelModel& m, const MouseState& mouse) const {
    for (int i = 0; i < static_cast<int>(m.results.size()) && i < 4; i++) {
        const TowerType& t = m.shop->type(m.results[i]);
        SDL_FRect r = cardRect(i);
        bool hover = mouse.x >= r.x && mouse.x < r.x + r.w && mouse.y >= r.y && mouse.y < r.y + r.h;
        bool active = m.placingType == t.typeId;
        bool affordable = m.gold >= t.cost;

        if (active) g.glow(r.x + r.w * 0.5f, r.y + r.h * 0.5f, r.w * 0.62f, r.h * 0.9f, withAlpha(pal::gold, 50));
        Color border = active ? pal::gold : (hover ? rgba(96, 108, 140) : rgba(50, 57, 78));
        g.fillRoundRect(r.x - 1.0f, r.y - 1.0f, r.w + 2.0f, r.h + 2.0f, 10.0f, border);
        Color top = hover || active ? rgba(52, 60, 84) : rgba(40, 46, 66);
        g.fillRoundRectGradient(r.x, r.y, r.w, r.h, 9.0f, top, rgba(28, 33, 48));

        float ix = r.x + 25.0f;
        float iy = r.y + r.h * 0.5f;
        g.fillCircleGradient(ix, iy, 20.0f, rgba(70, 92, 66), rgba(38, 52, 40));
        Tower::drawModel(g, t.typeId, ix, iy, 0.72f, -PI_F * 0.5f + 0.35f * std::sin(g.time() * 1.2f + i), 1, 0.0f, true);

        g.text(FontId::BodyMedium, t.title, r.x + 52.0f, r.y + 6.0f, affordable ? pal::uiText : pal::uiMuted);
        char buf[48];
        std::snprintf(buf, sizeof(buf), "DMG %d  RNG %d", t.damage, static_cast<int>(t.range));
        g.text(FontId::Small, buf, r.x + 52.0f, r.y + 27.0f, pal::uiMuted);

        std::string cost = std::to_string(t.cost);
        float cw = g.textWidth(FontId::BodyBold, cost);
        g.text(FontId::BodyBold, cost, r.x + r.w - 9.0f, r.y + 6.0f, affordable ? pal::gold : pal::danger, Align::Right);
        icons::coin(g, r.x + r.w - 17.0f - cw, r.y + 15.0f, 5.0f);
        g.text(FontId::SmallBold, std::to_string(t.typeId + 1), r.x + r.w - 9.0f, r.y + 29.0f, rgba(110, 118, 140), Align::Right);
    }
}

void SidePanel::drawStatRow(Gfx& g, float y, const std::string& label, const std::string& value) const {
    g.text(FontId::Small, label, x0 + 10.0f, y, pal::uiMuted);
    g.text(FontId::BodyMedium, value, x0 + width - 10.0f, y - 2.0f, pal::uiText, Align::Right);
}

void SidePanel::drawInfo(Gfx& g, const PanelModel& m, const MouseState& mouse) const {
    float top = infoTop;
    g.fillRoundRect(x0 - 1.0f, top - 1.0f, width + 2.0f, 140.0f, 10.0f, rgba(46, 52, 72));
    g.fillRoundRectGradient(x0, top, width, 138.0f, 9.0f, rgba(22, 26, 38), rgba(17, 20, 30));

    if (m.selected) {
        const Tower& t = *m.selected;
        g.text(FontId::HeadingSmall, t.getType().title, x0 + 10.0f, top + 9.0f, pal::gold);
        for (int i = 0; i < Tower::MAX_LEVEL; i++) {
            float px = x0 + width - 14.0f - (Tower::MAX_LEVEL - 1 - i) * 11.0f;
            float py = top + 17.0f;
            Color c = i < t.getLevel() ? pal::gold : rgba(70, 76, 96);
            g.fillPolygon({{px, py - 4.0f}, {px + 4.0f, py}, {px, py + 4.0f}, {px - 4.0f, py}}, c);
        }
        drawStatRow(g, top + 31.0f, "Damage", std::to_string(t.damage()));
        drawStatRow(g, top + 47.0f, "Range", std::to_string(static_cast<int>(t.range())));
        drawStatRow(g, top + 63.0f, "Fire rate", fmt("%.1f / s", t.fireRate()));
        const AmmoStore& a = t.getAmmo();
        g.text(FontId::Small, "Supply", x0 + 10.0f, top + 79.0f, pal::uiMuted);
        char buf[24];
        std::snprintf(buf, sizeof(buf), "%d / %d", a.current(), a.max());
        g.text(FontId::Small, buf, x0 + width - 10.0f, top + 79.0f, pal::uiText, Align::Right);
        float barX = x0 + 56.0f;
        float barW = width - 112.0f;
        g.fillRoundRect(barX, top + 85.0f, barW, 5.0f, 2.5f, rgba(8, 10, 16));
        g.fillRoundRect(barX, top + 85.0f, std::max(3.0f, barW * a.fraction()), 5.0f, 2.5f,
                        a.empty() ? pal::danger : pal::supply);

        Button up = upgradeButton;
        Button sell = sellButton;
        if (t.canUpgrade()) {
            up.label = "Upgrade " + std::to_string(t.upgradeCost());
            up.enabled = m.gold >= t.upgradeCost();
        } else {
            up.label = "Max level";
            up.enabled = false;
        }
        sell.label = "Sell +" + std::to_string(t.sellValue());
        up.draw(g, mouse);
        sell.draw(g, mouse);
        return;
    }

    int showType = cardAt(mouse.x, mouse.y, m);
    if (showType < 0) showType = m.placingType;
    if (showType >= 0) {
        const TowerType& t = m.shop->type(showType);
        g.text(FontId::HeadingSmall, t.title, x0 + 10.0f, top + 9.0f, pal::gold);
        std::string cost = std::to_string(t.cost);
        g.text(FontId::BodyBold, cost, x0 + width - 10.0f, top + 7.0f, m.gold >= t.cost ? pal::gold : pal::danger, Align::Right);
        icons::coin(g, x0 + width - 18.0f - g.textWidth(FontId::BodyBold, cost), top + 16.0f, 5.0f);
        float ly = top + 32.0f;
        for (const std::string& line : wrapText(g, FontId::Small, t.blurb, width - 20.0f)) {
            g.text(FontId::Small, line, x0 + 10.0f, ly, rgba(200, 204, 216));
            ly += 15.0f;
        }
        drawStatRow(g, top + 66.0f, "Damage", std::to_string(t.damage) + (t.splash > 0.0f ? " splash" : ""));
        drawStatRow(g, top + 82.0f, "Range", std::to_string(static_cast<int>(t.range)));
        drawStatRow(g, top + 98.0f, "Fire rate", fmt("%.1f / s", t.fireRate));
        if (m.placingType == showType) {
            g.textCentered(FontId::SmallBold, "Click grass to build", x0 + width * 0.5f, top + 124.0f, pal::gold);
        }
        return;
    }

    g.text(FontId::HeadingSmall, "Commander", x0 + 10.0f, top + 9.0f, pal::gold);
    const std::string hint =
        "Pick a tower, then click open grass to build. Enemies always take the shortest road to the castle. Bend their "
        "route with towers, but you can never seal it.";
    float ly = top + 32.0f;
    for (const std::string& line : wrapText(g, FontId::Small, hint, width - 20.0f)) {
        g.text(FontId::Small, line, x0 + 10.0f, ly, rgba(200, 204, 216));
        ly += 15.0f;
    }
}
