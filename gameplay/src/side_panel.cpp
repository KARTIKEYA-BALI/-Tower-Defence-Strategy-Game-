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
    : x0(PANEL_X + 14.0f), width(PANEL_W - 28.0f), cardsTop(232.0f), cardHeight(46.0f), cardGap(4.0f), infoTop(432.0f),
      spellHeight(60.0f), spellGap(6.0f) {
    const float half = (width - 6.0f) * 0.5f;
    waveButton = Button({x0, 122.0f, width, 44.0f}, "Start Wave", ButtonStyle::Primary);
    searchBox = TextBox({x0, 197.0f, width, 28.0f}, "Search towers...");
    ammoButton = Button({x0, 540.0f, half, 28.0f}, "Ammo", ButtonStyle::Secondary);
    repairButton = Button({x0 + half + 6.0f, 540.0f, half, 28.0f}, "Repair", ButtonStyle::Secondary);
    upgradeButton = Button({x0, 574.0f, half, 30.0f}, "Upgrade", ButtonStyle::Primary);
    sellButton = Button({x0 + half + 6.0f, 574.0f, half, 30.0f}, "Sell", ButtonStyle::Danger);
    saveButton = Button({x0, 616.0f, half, 30.0f}, "Save", ButtonStyle::Secondary);
    loadButton = Button({x0 + half + 6.0f, 616.0f, half, 30.0f}, "Load", ButtonStyle::Secondary);
    speedButton = Button({x0, 652.0f, half, 30.0f}, "Speed 1x", ButtonStyle::Secondary);
    pauseButton = Button({x0 + half + 6.0f, 652.0f, half, 30.0f}, "Pause", ButtonStyle::Secondary);
}

SDL_FRect SidePanel::cardRect(int slot) const {
    return {x0, cardsTop + slot * (cardHeight + cardGap), width, cardHeight};
}

SDL_FRect SidePanel::tabRect(int tab) const {
    float third = width / 3.0f;
    return {x0 + tab * third, 168.0f, third, 24.0f};
}

bool SidePanel::troopCardHit(float mx, float my, const PanelModel& m) const {
    if (m.tab != 2) return false;
    SDL_FRect r = cardRect(0);
    return mx >= r.x && mx < r.x + r.w && my >= r.y && my < r.y + r.h;
}

SDL_FRect SidePanel::spellRect(int slot) const {
    return {x0, cardsTop + slot * (spellHeight + spellGap), width, spellHeight};
}

SDL_FRect SidePanel::chipRect(int slot, int tier) const {
    SDL_FRect r = spellRect(slot);
    float cw = (r.w - 16.0f - 2.0f * 4.0f) / SPELL_TIERS;
    return {r.x + 8.0f + tier * (cw + 4.0f), r.y + 39.0f, cw, 16.0f};
}

int SidePanel::spellCardAt(float mx, float my, const PanelModel& m) const {
    if (m.tab != 1) return -1;
    for (int i = 0; i < static_cast<int>(m.spellResults.size()); i++) {
        SDL_FRect r = spellRect(i);
        if (mx >= r.x && mx < r.x + r.w && my >= r.y && my < r.y + r.h) return m.spellResults[i];
    }
    return -1;
}

int SidePanel::spellChipAt(float mx, float my, const PanelModel& m) const {
    if (m.tab != 1) return -1;
    for (int i = 0; i < static_cast<int>(m.spellResults.size()); i++) {
        for (int t = 0; t < SPELL_TIERS; t++) {
            SDL_FRect r = chipRect(i, t);
            if (mx >= r.x && mx < r.x + r.w && my >= r.y && my < r.y + r.h) return m.spellResults[i] * SPELL_TIERS + t;
        }
    }
    return -1;
}

int SidePanel::cardAt(float mx, float my, const PanelModel& m) const {
    if (m.tab != 0) return -1;
    for (int i = 0; i < static_cast<int>(m.results.size()); i++) {
        SDL_FRect r = cardRect(i);
        if (mx >= r.x && mx < r.x + r.w && my >= r.y && my < r.y + r.h) return m.results[i];
    }
    return -1;
}

PanelHit SidePanel::hitTest(float mx, float my, const PanelModel& m) const {
    if (waveButton.contains(mx, my) && !m.waveActive && !m.allWavesDone) return {PanelAction::StartWave, -1};
    if (searchBox.contains(mx, my)) return {PanelAction::FocusSearch, -1};
    for (int tab = 0; tab < 3; tab++) {
        SDL_FRect r = tabRect(tab);
        if (mx >= r.x && mx < r.x + r.w && my >= r.y && my < r.y + r.h)
            return {tab == 0 ? PanelAction::TabTowers : (tab == 1 ? PanelAction::TabSpells : PanelAction::TabTroops), -1};
    }
    if (troopCardHit(mx, my, m)) return {PanelAction::PickTroop, -1};
    if (m.troop) {
        SDL_FRect d = {x0, sellButton.rect.y, width, sellButton.rect.h};
        if (mx >= d.x && mx < d.x + d.w && my >= d.y && my < d.y + d.h) return {PanelAction::Sell, -1};
    }
    int chip = spellChipAt(mx, my, m);
    if (chip >= 0) return {PanelAction::PickSpell, chip};
    int spell = spellCardAt(mx, my, m);
    if (spell >= 0) {
        int tier = (m.placingSpell == spell && SpellBook::validTier(m.placingTier)) ? m.placingTier : 0;
        return {PanelAction::PickSpell, spell * SPELL_TIERS + tier};
    }
    int card = cardAt(mx, my, m);
    if (card >= 0) return {PanelAction::PickType, card};
    if (m.selected) {
        if (upgradeButton.contains(mx, my) && m.selected->canUpgrade()) return {PanelAction::Upgrade, -1};
        if (sellButton.contains(mx, my)) return {PanelAction::Sell, -1};
        if (ammoButton.contains(mx, my)) return {PanelAction::AmmoDrone, -1};
        if (repairButton.contains(mx, my)) return {PanelAction::RepairDrone, -1};
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

    drawTabs(g, m, mouse);
    TextBox box = searchBox;
    box.placeholder = m.tab == 1 ? "Search spells...  ( / )" : "Search towers...  ( / )";
    box.draw(g, m.search, m.searchFocused, mouse);
    if (m.tab == 1) drawSpells(g, m, mouse);
    else if (m.tab == 2) drawTroops(g, m, mouse);
    else drawCards(g, m, mouse);
    if (m.troop) drawTroopInfo(g, m, mouse);
    else if (m.tab == 1 && !m.selected) drawSpellInfo(g, m, mouse);
    else if (m.tab == 2 && !m.selected) drawTroopInfo(g, m, mouse);
    else drawInfo(g, m, mouse);

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
    if (m.results.empty()) {
        g.textCentered(FontId::Body, "No tower matches", x0 + width * 0.5f, cardsTop + 24.0f, pal::uiMuted);
        g.textCentered(FontId::BodyBold, "\"" + m.search + "\"", x0 + width * 0.5f, cardsTop + 44.0f, pal::uiText);
        return;
    }
    for (int i = 0; i < static_cast<int>(m.results.size()) && i < 4; i++) {
        const TowerType& t = m.shop->type(m.results[i]);
        SDL_FRect r = cardRect(i);
        bool hover = mouse.x >= r.x && mouse.x < r.x + r.w && mouse.y >= r.y && mouse.y < r.y + r.h;
        bool active = m.placingType == t.typeId;
        bool affordable = m.gold >= t.cost;

        if (active) g.glow(r.x + r.w * 0.5f, r.y + r.h * 0.5f, r.w * 0.62f, r.h * 0.9f, withAlpha(pal::gold, 50));
        Color border = active ? pal::gold : (hover ? rgba(96, 108, 140) : rgba(50, 57, 78));
        g.fillRoundRect(r.x - 1.0f, r.y + 2.0f, r.w + 2.0f, r.h + 2.0f, 10.0f, rgba(12, 14, 22));
        g.fillRoundRect(r.x - 1.0f, r.y - 1.0f, r.w + 2.0f, r.h + 2.0f, 10.0f, border);
        Color top = hover || active ? rgba(52, 60, 84) : rgba(40, 46, 66);
        g.fillRoundRectGradient(r.x, r.y, r.w, r.h, 9.0f, top, rgba(28, 33, 48));

        float ix = r.x + 25.0f;
        float iy = r.y + r.h * 0.5f;
        g.fillCircleGradient(ix, iy, 19.0f, rgba(70, 92, 66), rgba(38, 52, 40));
        Tower::drawIcon(g, t.typeId, ix, iy + 2.0f, 0.6f, -PI_F * 0.25f + 0.35f * std::sin(g.time() * 1.2f + i));

        g.text(FontId::BodyMedium, t.title, r.x + 52.0f, r.y + 5.0f, affordable ? pal::uiText : pal::uiMuted);
        char buf[48];
        std::snprintf(buf, sizeof(buf), "DMG %d  RNG %d", t.damage, static_cast<int>(t.range));
        g.text(FontId::Small, buf, r.x + 52.0f, r.y + 25.0f, pal::uiMuted);

        std::string cost = std::to_string(t.cost);
        float cw = g.textWidth(FontId::BodyBold, cost);
        g.text(FontId::BodyBold, cost, r.x + r.w - 9.0f, r.y + 5.0f, affordable ? pal::gold : pal::danger, Align::Right);
        icons::coin(g, r.x + r.w - 17.0f - cw, r.y + 14.0f, 5.0f);
        g.text(FontId::SmallBold, std::to_string(t.typeId + 1), r.x + r.w - 9.0f, r.y + 27.0f, rgba(110, 118, 140), Align::Right);
    }
}

void SidePanel::drawTabs(Gfx& g, const PanelModel& m, const MouseState& mouse) const {
    const char* labels[3] = {"TOWERS", "SPELLS", "TROOPS"};
    for (int tab = 0; tab < 3; tab++) {
        SDL_FRect r = tabRect(tab);
        bool active = tab == m.tab;
        bool hover = mouse.x >= r.x && mouse.x < r.x + r.w && mouse.y >= r.y && mouse.y < r.y + r.h;
        if (active) g.fillRoundRect(r.x + 1.0f, r.y, r.w - 2.0f, r.h, 6.0f, rgba(52, 46, 30));
        else if (hover) g.fillRoundRect(r.x + 1.0f, r.y, r.w - 2.0f, r.h, 6.0f, rgba(40, 46, 64));
        g.textCentered(FontId::SmallBold, labels[tab], r.x + r.w * 0.5f, r.y + r.h * 0.5f,
                       active ? pal::gold : (hover ? pal::uiText : pal::uiMuted));
        g.fillRect(r.x + 4.0f, r.y + r.h, r.w - 8.0f, active ? 2.0f : 1.0f, active ? pal::gold : rgba(60, 68, 92));
    }
}

void SidePanel::drawSpells(Gfx& g, const PanelModel& m, const MouseState& mouse) const {
    if (m.spellResults.empty()) {
        g.textCentered(FontId::Body, "No spell matches", x0 + width * 0.5f, cardsTop + 24.0f, pal::uiMuted);
        g.textCentered(FontId::BodyBold, "\"" + m.search + "\"", x0 + width * 0.5f, cardsTop + 44.0f, pal::uiText);
        return;
    }
    for (int i = 0; i < static_cast<int>(m.spellResults.size()) && i < SPELL_KINDS; i++) {
        int kind = m.spellResults[i];
        const SpellType& s = m.spells->type(kind);
        SDL_FRect r = spellRect(i);
        bool hover = mouse.x >= r.x && mouse.x < r.x + r.w && mouse.y >= r.y && mouse.y < r.y + r.h;
        bool armed = m.placingSpell == kind;
        bool ready = m.spells->ready(kind);

        if (armed) g.glow(r.x + r.w * 0.5f, r.y + r.h * 0.5f, r.w * 0.62f, r.h * 0.9f, withAlpha(s.colour, 50));
        Color border = armed ? s.colour : (hover ? rgba(96, 108, 140) : rgba(50, 57, 78));
        g.fillRoundRect(r.x - 1.0f, r.y + 2.0f, r.w + 2.0f, r.h + 2.0f, 10.0f, rgba(12, 14, 22));
        g.fillRoundRect(r.x - 1.0f, r.y - 1.0f, r.w + 2.0f, r.h + 2.0f, 10.0f, border);
        g.fillRoundRectGradient(r.x, r.y, r.w, r.h, 9.0f, hover || armed ? rgba(52, 60, 84) : rgba(40, 46, 66),
                                rgba(28, 33, 48));
        SpellBook::drawIcon(g, kind, r.x + 22.0f, r.y + 20.0f, 14.0f, g.time());
        g.text(FontId::BodyMedium, s.title, r.x + 44.0f, r.y + 3.0f, ready ? pal::uiText : pal::uiMuted);
        char buf[48];
        if (ready) std::snprintf(buf, sizeof(buf), "%s", s.effect.c_str());
        else std::snprintf(buf, sizeof(buf), "Ready in %ds", static_cast<int>(std::ceil(m.spells->cooldownLeft(kind))));
        g.text(FontId::Small, buf, r.x + 44.0f, r.y + 21.0f, ready ? pal::uiMuted : rgba(236, 150, 120));
        const char keys[3] = {'Z', 'X', 'C'};
        g.text(FontId::SmallBold, std::string(1, keys[kind % 3]), r.x + r.w - 9.0f, r.y + 5.0f, rgba(110, 118, 140), Align::Right);

        for (int t = 0; t < SPELL_TIERS; t++) {
            SDL_FRect c = chipRect(i, t);
            bool chipHover = mouse.x >= c.x && mouse.x < c.x + c.w && mouse.y >= c.y && mouse.y < c.y + c.h;
            bool chipArmed = armed && m.placingTier == t;
            bool afford = m.gold >= s.tiers[t].cost;
            Color fill = chipArmed ? shade(s.colour, 0.55f) : (chipHover ? rgba(64, 72, 98) : rgba(30, 35, 50));
            g.fillRoundRect(c.x, c.y, c.w, c.h, 5.0f, fill);
            g.strokeRoundRect(c.x, c.y, c.w, c.h, 5.0f, 1.0f, chipArmed ? s.colour : rgba(70, 78, 102));
            std::string label = std::string(SpellBook::tierName(t)) + "  " + std::to_string(s.tiers[t].cost);
            g.textCentered(FontId::SmallBold, label, c.x + c.w * 0.5f, c.y + c.h * 0.5f,
                           afford && ready ? (chipArmed ? rgba(255, 255, 255) : pal::gold) : rgba(150, 100, 96));
        }

        if (!ready) {
            float f = m.spells->cooldownFraction(kind);
            g.fillRoundRect(r.x + 6.0f, r.y + r.h - 3.0f, (r.w - 12.0f), 2.0f, 1.0f, rgba(10, 12, 18));
            g.fillRoundRect(r.x + 6.0f, r.y + r.h - 3.0f, std::max(2.0f, (r.w - 12.0f) * (1.0f - f)), 2.0f, 1.0f, s.colour);
        }
    }
}

void SidePanel::drawSpellInfo(Gfx& g, const PanelModel& m, const MouseState& mouse) const {
    float top = infoTop;
    g.fillRoundRect(x0 - 1.0f, top - 1.0f, width + 2.0f, 180.0f, 10.0f, rgba(46, 52, 72));
    g.fillRoundRectGradient(x0, top, width, 178.0f, 9.0f, rgba(22, 26, 38), rgba(17, 20, 30));

    int kind = -1;
    int tier = -1;
    int chip = spellChipAt(mouse.x, mouse.y, m);
    if (chip >= 0) {
        kind = chip / SPELL_TIERS;
        tier = chip % SPELL_TIERS;
    } else if (spellCardAt(mouse.x, mouse.y, m) >= 0) {
        kind = spellCardAt(mouse.x, mouse.y, m);
        tier = (m.placingSpell == kind) ? m.placingTier : 0;
    } else if (m.placingSpell >= 0) {
        kind = m.placingSpell;
        tier = m.placingTier;
    }

    if (kind < 0 || !m.spells->validKind(kind) || !SpellBook::validTier(tier)) {
        g.text(FontId::HeadingSmall, "Spellbook", x0 + 10.0f, top + 9.0f, pal::gold);
        const std::string hint =
            "Pick a spell and a tier, then click the map. Each cast costs gold and the spell then needs time to recharge. "
            "Tier II and III last longer.";
        float ly = top + 32.0f;
        for (const std::string& line : wrapText(g, FontId::Small, hint, width - 20.0f)) {
            g.text(FontId::Small, line, x0 + 10.0f, ly, rgba(200, 204, 216));
            ly += 15.0f;
        }
        return;
    }

    const SpellType& s = m.spells->type(kind);
    const SpellTier& st = s.tiers[tier];
    std::string title = s.title + " " + SpellBook::tierName(tier);
    g.text(FontId::HeadingSmall, title, x0 + 10.0f, top + 9.0f, s.colour);
    std::string cost = std::to_string(st.cost);
    g.text(FontId::BodyBold, cost, x0 + width - 10.0f, top + 7.0f, m.gold >= st.cost ? pal::gold : pal::danger, Align::Right);
    icons::coin(g, x0 + width - 18.0f - g.textWidth(FontId::BodyBold, cost), top + 16.0f, 5.0f);
    float ly = top + 32.0f;
    for (const std::string& line : wrapText(g, FontId::Small, s.blurb, width - 20.0f)) {
        g.text(FontId::Small, line, x0 + 10.0f, ly, rgba(200, 204, 216));
        ly += 15.0f;
    }
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%.1f s", st.duration);
    drawStatRow(g, top + 82.0f, "Lasts", buf);
    drawStatRow(g, top + 98.0f, "Radius", std::to_string(s.radius) + (s.radius == 1 ? " cell" : " cells"));
    drawStatRow(g, top + 114.0f, "Effect", s.effect);
    std::snprintf(buf, sizeof(buf), "%.0f s", s.cooldown);
    drawStatRow(g, top + 130.0f, "Recharge", buf);
    if (m.placingSpell == kind) {
        g.textCentered(FontId::SmallBold, "Click the map to cast", x0 + width * 0.5f, top + 160.0f, s.colour);
    }
}

void SidePanel::drawTroops(Gfx& g, const PanelModel& m, const MouseState& mouse) const {
    SDL_FRect r = cardRect(0);
    bool hover = mouse.x >= r.x && mouse.x < r.x + r.w && mouse.y >= r.y && mouse.y < r.y + r.h;
    bool active = m.placingTroop;
    bool full = m.troopCount >= m.maxTroops;
    bool affordable = m.gold >= m.troopCost && !full;
    if (active) g.glow(r.x + r.w * 0.5f, r.y + r.h * 0.5f, r.w * 0.62f, r.h * 0.9f, withAlpha(pal::supply, 50));
    Color border = active ? pal::supply : (hover ? rgba(96, 108, 140) : rgba(50, 57, 78));
    g.fillRoundRect(r.x - 1.0f, r.y + 2.0f, r.w + 2.0f, r.h + 2.0f, 10.0f, rgba(14, 16, 24));
    g.fillRoundRect(r.x - 1.0f, r.y - 1.0f, r.w + 2.0f, r.h + 2.0f, 10.0f, border);
    g.fillRoundRectGradient(r.x, r.y, r.w, r.h, 9.0f, hover || active ? rgba(52, 60, 84) : rgba(40, 46, 66), rgba(28, 33, 48));
    float ix = r.x + 25.0f;
    float iy = r.y + r.h * 0.5f;
    g.fillCircleGradient(ix, iy, 19.0f, rgba(70, 92, 66), rgba(38, 52, 40));
    Troop::drawIcon(g, ix, iy + 2.0f, 0.55f);
    g.text(FontId::BodyMedium, "Cannon Cart", r.x + 52.0f, r.y + 5.0f, affordable ? pal::uiText : pal::uiMuted);
    char buf[48];
    std::snprintf(buf, sizeof(buf), "DMG %d  RNG %d", Troop::DAMAGE, static_cast<int>(Troop::RANGE));
    g.text(FontId::Small, buf, r.x + 52.0f, r.y + 25.0f, pal::uiMuted);
    std::string cost = std::to_string(m.troopCost);
    float cw = g.textWidth(FontId::BodyBold, cost);
    g.text(FontId::BodyBold, cost, r.x + r.w - 9.0f, r.y + 5.0f, m.gold >= m.troopCost ? pal::gold : pal::danger, Align::Right);
    icons::coin(g, r.x + r.w - 17.0f - cw, r.y + 14.0f, 5.0f);
    g.text(FontId::SmallBold, "T", r.x + r.w - 9.0f, r.y + 27.0f, rgba(110, 118, 140), Align::Right);
    std::snprintf(buf, sizeof(buf), "On the field: %d / %d", m.troopCount, m.maxTroops);
    g.textCentered(FontId::SmallBold, buf, x0 + width * 0.5f, cardsTop + cardHeight + 18.0f, full ? pal::danger : pal::supply);
    const std::string hint = "A cannon on wheels. It drives to any Raider that attacks your towers, fights near its post, then drives home.";
    float ly = cardsTop + cardHeight + 36.0f;
    for (const std::string& line : wrapText(g, FontId::Small, hint, width - 8.0f)) {
        g.text(FontId::Small, line, x0 + 4.0f, ly, rgba(200, 204, 216));
        ly += 15.0f;
    }
}

void SidePanel::drawTroopInfo(Gfx& g, const PanelModel& m, const MouseState& mouse) const {
    float top = infoTop;
    g.fillRoundRect(x0 - 1.0f, top - 1.0f, width + 2.0f, 180.0f, 10.0f, rgba(46, 52, 72));
    g.fillRoundRectGradient(x0, top, width, 178.0f, 9.0f, rgba(22, 26, 38), rgba(17, 20, 30));
    g.text(FontId::HeadingSmall, "Cannon Cart", x0 + 10.0f, top + 9.0f, pal::supply);
    char buf[48];
    if (m.troop) {
        const Troop& t = *m.troop;
        float barX = x0 + 56.0f;
        float barW = width - 112.0f;
        g.text(FontId::Small, "Health", x0 + 10.0f, top + 32.0f, pal::uiMuted);
        std::snprintf(buf, sizeof(buf), "%d / %d", t.getHealth(), t.getMaxHealth());
        g.text(FontId::Small, buf, x0 + width - 10.0f, top + 32.0f, pal::uiText, Align::Right);
        g.fillRoundRect(barX, top + 38.0f, barW, 5.0f, 2.5f, rgba(8, 10, 16));
        g.fillRoundRect(barX, top + 38.0f, std::max(3.0f, barW * t.healthFraction()), 5.0f, 2.5f,
                        mix(pal::danger, pal::supply, t.healthFraction()));
        drawStatRow(g, top + 52.0f, "Damage", std::to_string(Troop::DAMAGE) + " splash");
        drawStatRow(g, top + 68.0f, "Range", std::to_string(static_cast<int>(Troop::RANGE)));
        g.textCentered(FontId::SmallBold, t.modeName(), x0 + width * 0.5f, top + 96.0f,
                       t.getMode() == TroopMode::Hunt ? pal::danger : pal::gold);
        g.textCentered(FontId::Small, "Heals slowly while it guards", x0 + width * 0.5f, top + 114.0f, pal::uiMuted);
        Button dismiss = sellButton;
        dismiss.rect.x = x0;
        dismiss.rect.w = width;
        dismiss.label = "Dismiss +" + std::to_string(t.sellValue());
        dismiss.draw(g, mouse);
        return;
    }
    const std::string text = "Click open ground to park it. Its post is where it returns. It heals there over time.";
    float ly = top + 32.0f;
    for (const std::string& line : wrapText(g, FontId::Small, text, width - 20.0f)) {
        g.text(FontId::Small, line, x0 + 10.0f, ly, rgba(200, 204, 216));
        ly += 15.0f;
    }
    drawStatRow(g, top + 82.0f, "Health", std::to_string(Troop::HEALTH));
    std::snprintf(buf, sizeof(buf), "%.1f / s", 1.0f / Troop::RELOAD);
    drawStatRow(g, top + 98.0f, "Fire rate", buf);
    drawStatRow(g, top + 114.0f, "Guards", std::to_string(static_cast<int>(Troop::GUARD_RADIUS / CELL_SIZE)) + " cells");
    drawStatRow(g, top + 130.0f, "Max", std::to_string(m.maxTroops) + " at once");
    if (m.placingTroop) g.textCentered(FontId::SmallBold, "Click open ground", x0 + width * 0.5f, top + 160.0f, pal::supply);
}

void SidePanel::drawStatRow(Gfx& g, float y, const std::string& label, const std::string& value) const {
    g.text(FontId::Small, label, x0 + 10.0f, y, pal::uiMuted);
    g.text(FontId::BodyMedium, value, x0 + width - 10.0f, y - 2.0f, pal::uiText, Align::Right);
}

void SidePanel::drawInfo(Gfx& g, const PanelModel& m, const MouseState& mouse) const {
    float top = infoTop;
    g.fillRoundRect(x0 - 1.0f, top - 1.0f, width + 2.0f, 180.0f, 10.0f, rgba(46, 52, 72));
    g.fillRoundRectGradient(x0, top, width, 178.0f, 9.0f, rgba(22, 26, 38), rgba(17, 20, 30));

    if (m.selected) {
        const Tower& t = *m.selected;
        g.text(FontId::HeadingSmall, t.getType().title, x0 + 10.0f, top + 9.0f, pal::gold);
        for (int i = 0; i < Tower::MAX_LEVEL; i++) {
            float px = x0 + width - 14.0f - (Tower::MAX_LEVEL - 1 - i) * 11.0f;
            float py = top + 17.0f;
            Color c = i < t.getLevel() ? pal::gold : rgba(70, 76, 96);
            g.fillPolygon({{px, py - 4.0f}, {px + 4.0f, py}, {px, py + 4.0f}, {px - 4.0f, py}}, c);
        }
        char buf[24];
        float barX = x0 + 56.0f;
        float barW = width - 112.0f;
        g.text(FontId::Small, "Health", x0 + 10.0f, top + 28.0f, pal::uiMuted);
        std::snprintf(buf, sizeof(buf), "%d / %d", t.getHealth(), t.getMaxHealth());
        g.text(FontId::Small, buf, x0 + width - 10.0f, top + 28.0f, pal::uiText, Align::Right);
        g.fillRoundRect(barX, top + 34.0f, barW, 5.0f, 2.5f, rgba(8, 10, 16));
        g.fillRoundRect(barX, top + 34.0f, std::max(3.0f, barW * t.healthFraction()), 5.0f, 2.5f,
                        mix(pal::danger, pal::good, t.healthFraction()));
        drawStatRow(g, top + 44.0f, "Damage", std::to_string(t.damage()));
        drawStatRow(g, top + 59.0f, "Range", std::to_string(static_cast<int>(t.range())));
        drawStatRow(g, top + 74.0f, "Fire rate", fmt("%.1f / s", t.fireRate()));
        const AmmoStore& a = t.getAmmo();
        g.text(FontId::Small, "Ammo", x0 + 10.0f, top + 89.0f, pal::uiMuted);
        std::snprintf(buf, sizeof(buf), "%d / %d", a.current(), a.max());
        g.text(FontId::Small, buf, x0 + width - 10.0f, top + 89.0f, pal::uiText, Align::Right);
        g.fillRoundRect(barX, top + 95.0f, barW, 5.0f, 2.5f, rgba(8, 10, 16));
        g.fillRoundRect(barX, top + 95.0f, std::max(3.0f, barW * a.fraction()), 5.0f, 2.5f,
                        a.empty() ? pal::danger : pal::supply);

        bool busy = m.dronesActive >= m.maxDrones;
        Button ammoDrone = ammoButton;
        Button repairDrone = repairButton;
        ammoDrone.label = busy ? "Busy" : "Ammo " + std::to_string(m.ammoDroneCost);
        repairDrone.label = busy ? "Busy" : "Repair " + std::to_string(m.repairDroneCost);
        ammoDrone.enabled = !busy && !a.full() && m.gold >= m.ammoDroneCost;
        repairDrone.enabled = !busy && t.getHealth() < t.getMaxHealth() && m.gold >= m.repairDroneCost;
        ammoDrone.draw(g, mouse);
        repairDrone.draw(g, mouse);

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
        drawStatRow(g, top + 114.0f, "Ammo", std::to_string(t.ammoMax) + " shots");
        drawStatRow(g, top + 130.0f, "Health", std::to_string(t.health));
        if (m.placingType == showType) {
            g.textCentered(FontId::SmallBold, "Click grass to build", x0 + width * 0.5f, top + 160.0f, pal::gold);
        }
        return;
    }

    g.text(FontId::HeadingSmall, "Commander", x0 + 10.0f, top + 9.0f, pal::gold);
    const std::string hint =
        "Pick a tower, then click open grass to build. Every enemy finds its own way to the castle, and Raiders "
        "smash towers. Ammo never reloads: select a tower and send drones (A ammo, R repair).";
    float ly = top + 32.0f;
    for (const std::string& line : wrapText(g, FontId::Small, hint, width - 20.0f)) {
        g.text(FontId::Small, line, x0 + 10.0f, ly, rgba(200, 204, 216));
        ly += 15.0f;
    }
}
