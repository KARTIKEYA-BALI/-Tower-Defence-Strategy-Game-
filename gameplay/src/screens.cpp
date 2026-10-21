#include "screens.h"
#include "game_config.h"
#include <cmath>
#include <cstdio>

namespace {

void ornament(Gfx& g, float cx, float y, float halfWidth) {
    g.fillPolygonColors({{cx - halfWidth, y - 0.7f}, {cx - 12.0f, y - 0.7f}, {cx - 12.0f, y + 0.7f}, {cx - halfWidth, y + 0.7f}},
                        {withAlpha(pal::gold, 0), withAlpha(pal::gold, 200), withAlpha(pal::gold, 200), withAlpha(pal::gold, 0)});
    g.fillPolygonColors({{cx + 12.0f, y - 0.7f}, {cx + halfWidth, y - 0.7f}, {cx + halfWidth, y + 0.7f}, {cx + 12.0f, y + 0.7f}},
                        {withAlpha(pal::gold, 200), withAlpha(pal::gold, 0), withAlpha(pal::gold, 0), withAlpha(pal::gold, 200)});
    g.fillPolygon({{cx, y - 5.0f}, {cx + 5.0f, y}, {cx, y + 5.0f}, {cx - 5.0f, y}}, pal::gold);
    g.fillPolygon({{cx, y - 2.2f}, {cx + 2.2f, y}, {cx, y + 2.2f}, {cx - 2.2f, y}}, rgba(40, 30, 10));
}

}

Screens::Screens() {
    const float cx = WINDOW_W * 0.5f;
    const float bw = 260.0f;
    newGame = Button({cx - bw * 0.5f, 340.0f, bw, 48.0f}, "New Game", ButtonStyle::Primary);
    continueGame = Button({cx - bw * 0.5f, 400.0f, bw, 44.0f}, "Continue", ButtonStyle::Secondary);
    help = Button({cx - bw * 0.5f, 456.0f, bw, 44.0f}, "How to Play", ButtonStyle::Secondary);
    quit = Button({cx - bw * 0.5f, 512.0f, bw, 44.0f}, "Quit", ButtonStyle::Secondary);

    const float pw = 220.0f;
    resume = Button({cx - pw * 0.5f, 300.0f, pw, 44.0f}, "Resume", ButtonStyle::Primary);
    restartPause = Button({cx - pw * 0.5f, 354.0f, pw, 40.0f}, "Restart Game", ButtonStyle::Secondary);
    menuPause = Button({cx - pw * 0.5f, 404.0f, pw, 40.0f}, "Main Menu", ButtonStyle::Secondary);

    restartEnd = Button({cx - 200.0f, 432.0f, 190.0f, 44.0f}, "Restart Game", ButtonStyle::Primary);
    menuEnd = Button({cx + 10.0f, 432.0f, 190.0f, 44.0f}, "Main Menu", ButtonStyle::Secondary);

    closeHelp = Button({cx - 90.0f, 484.0f, 180.0f, 42.0f}, "Got it", ButtonStyle::Primary);
}

void Screens::dim(Gfx& g, int alpha) const {
    g.fillRect(0, 0, WINDOW_W, WINDOW_H, rgba(6, 8, 14, alpha));
}

ScreenAction Screens::menuHit(float mx, float my, bool hasSave) const {
    if (newGame.contains(mx, my)) return ScreenAction::NewGame;
    if (hasSave && continueGame.contains(mx, my)) return ScreenAction::Continue;
    if (help.contains(mx, my)) return ScreenAction::Help;
    if (quit.contains(mx, my)) return ScreenAction::Quit;
    return ScreenAction::None;
}

ScreenAction Screens::pauseHit(float mx, float my) const {
    if (resume.contains(mx, my)) return ScreenAction::Resume;
    if (restartPause.contains(mx, my)) return ScreenAction::Restart;
    if (menuPause.contains(mx, my)) return ScreenAction::MainMenu;
    return ScreenAction::None;
}

ScreenAction Screens::endHit(float mx, float my) const {
    if (restartEnd.contains(mx, my)) return ScreenAction::Restart;
    if (menuEnd.contains(mx, my)) return ScreenAction::MainMenu;
    return ScreenAction::None;
}

ScreenAction Screens::helpHit(float mx, float my) const {
    if (closeHelp.contains(mx, my)) return ScreenAction::CloseHelp;
    return ScreenAction::None;
}

void Screens::drawMenu(Gfx& g, const MouseState& mouse, bool hasSave) const {
    const float cx = WINDOW_W * 0.5f;
    g.fillPolygonColors({{0, 0}, {WINDOW_W, 0}, {WINDOW_W, WINDOW_H}, {0, WINDOW_H}},
                        {rgba(8, 10, 18, 150), rgba(8, 10, 18, 150), rgba(8, 10, 18, 235), rgba(8, 10, 18, 235)});
    g.glow(cx, 190.0f, 420.0f, 150.0f, rgba(8, 10, 18, 170));
    g.glow(cx, 175.0f, 300.0f, 70.0f, rgba(234, 186, 78, 26));

    g.textCentered(FontId::Title, "Siege and Supply", cx, 168.0f, pal::gold, true);
    ornament(g, cx, 222.0f, 200.0f);
    g.textCentered(FontId::Heading, "A Tower Defence Strategy Game", cx, 254.0f, rgba(226, 220, 204), true);

    Button cont = continueGame;
    cont.enabled = hasSave;
    newGame.draw(g, mouse);
    cont.draw(g, mouse);
    help.draw(g, mouse);
    quit.draw(g, mouse);

    g.textCentered(FontId::Small, "Bit Beaters   |   DSCPP-III-2026-T016   |   Graphic Era (Deemed to be University)", cx,
                   WINDOW_H - 22.0f, rgba(150, 156, 172));
}

void Screens::drawPause(Gfx& g, const MouseState& mouse) const {
    dim(g, 150);
    const float cx = WINDOW_W * 0.5f;
    panelCard(g, cx - 160.0f, 190.0f, 320.0f, 290.0f);
    g.textCentered(FontId::Heading, "Paused", cx, 232.0f, pal::gold, true);
    ornament(g, cx, 262.0f, 110.0f);
    resume.draw(g, mouse);
    restartPause.draw(g, mouse);
    menuPause.draw(g, mouse);
}

void Screens::drawEnd(Gfx& g, const MouseState& mouse, const EndStats& s) const {
    dim(g, 165);
    const float cx = WINDOW_W * 0.5f;
    panelCard(g, cx - 240.0f, 170.0f, 480.0f, 340.0f);
    if (s.victory) {
        g.glow(cx, 230.0f, 200.0f, 60.0f, rgba(234, 186, 78, 40));
        g.textCentered(FontId::HeadingLarge, "Victory", cx, 228.0f, pal::gold, true);
        g.textCentered(FontId::Body, "The siege is broken! Your castle still stands.", cx, 282.0f, pal::uiText);
    } else {
        g.glow(cx, 230.0f, 200.0f, 60.0f, rgba(220, 76, 66, 40));
        g.textCentered(FontId::HeadingLarge, "Defeat", cx, 228.0f, rgba(236, 110, 96), true);
        g.textCentered(FontId::Body, "The castle has fallen!", cx, 282.0f, pal::uiText);
    }
    ornament(g, cx, 258.0f, 150.0f);

    const char* labels[3] = {"Waves cleared", "Enemies defeated", "Towers built"};
    char values[3][24];
    std::snprintf(values[0], sizeof(values[0]), "%d / %d", s.wavesCleared, s.totalWaves);
    std::snprintf(values[1], sizeof(values[1]), "%d", s.enemiesDefeated);
    std::snprintf(values[2], sizeof(values[2]), "%d", s.towersBuilt);
    for (int i = 0; i < 3; i++) {
        float colX = cx - 150.0f + i * 150.0f;
        g.textCentered(FontId::Number, values[i], colX, 340.0f, pal::uiText);
        g.textCentered(FontId::Small, labels[i], colX, 364.0f, pal::uiMuted);
    }
    if (s.victory) {
        char buf[48];
        std::snprintf(buf, sizeof(buf), "Castle health remaining: %d", s.castleHealth);
        g.textCentered(FontId::Small, buf, cx, 398.0f, pal::gold);
    }
    restartEnd.draw(g, mouse);
    menuEnd.draw(g, mouse);
}

void Screens::drawHelp(Gfx& g, const MouseState& mouse) const {
    dim(g, 175);
    const float cx = WINDOW_W * 0.5f;
    panelCard(g, cx - 300.0f, 130.0f, 600.0f, 420.0f);
    g.textCentered(FontId::Heading, "How to Play", cx, 170.0f, pal::gold, true);
    ornament(g, cx, 198.0f, 160.0f);
    const char* lines[] = {
        "Enemies march from the purple portal to your castle. Do not let them in.",
        "Pick a tower in the Armory (or press 1-4), then click open grass to build.",
        "Enemies always take the shortest route. Towers bend that route, but you can never fully block it.",
        "Click a tower to see its range, upgrade it (U) or sell it (S) for half its cost.",
        "Towers carry limited ammo that slowly resupplies. Watch the blue bars.",
        "Space starts the next wave. F switches speed. P or Esc pauses.",
        "Save and Load keep your towers, gold, health and wave.",
    };
    float y = 224.0f;
    for (const char* line : lines) {
        std::vector<std::string> wrapped = wrapText(g, FontId::Body, line, 500.0f);
        g.fillPolygon({{cx - 262.0f, y + 6.0f}, {cx - 258.0f, y + 10.0f}, {cx - 262.0f, y + 14.0f}, {cx - 266.0f, y + 10.0f}}, pal::gold);
        for (const std::string& w : wrapped) {
            g.text(FontId::Body, w, cx - 248.0f, y, rgba(220, 222, 230));
            y += 20.0f;
        }
        y += 7.0f;
    }
    closeHelp.draw(g, mouse);
}
