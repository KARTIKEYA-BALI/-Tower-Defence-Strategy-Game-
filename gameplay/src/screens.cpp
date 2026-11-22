#include "screens.h"
#include "engine_core.h"
#include "game_config.h"
#include "profile_manager.h"
#include "record_book.h"
#include "theme.h"
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace {

const float MAP_LIST_X = 50.0f;
const float MAP_LIST_W = 350.0f;
const float MAP_ROWS_TOP = 166.0f;
const float MAP_ROW_H = 84.0f;
const float MAP_ROW_STEP = 89.0f;
const float MAP_DETAIL_X = 430.0f;
const float MAP_DETAIL_W = 520.0f;
const float BIG_CELL = 22.0f;

SDL_FRect mapRowRect(int i) {
    return {MAP_LIST_X, MAP_ROWS_TOP + i * MAP_ROW_STEP, MAP_LIST_W, MAP_ROW_H};
}

const float ROWS_TOP = 172.0f;
const float ROW_H = 44.0f;
const float ROW_STEP = 52.0f;

SDL_FRect profileRect(int i) {
    return {WINDOW_W * 0.5f - 240.0f, ROWS_TOP + i * ROW_STEP, 360.0f, ROW_H};
}

SDL_FRect deleteRect(int i) {
    return {WINDOW_W * 0.5f + 132.0f, ROWS_TOP + i * ROW_STEP, 108.0f, ROW_H};
}

bool inside(const SDL_FRect& r, float mx, float my) {
    return mx >= r.x && mx < r.x + r.w && my >= r.y && my < r.y + r.h;
}

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
    changeProfile = Button({cx - bw * 0.5f, 456.0f, bw, 44.0f}, "Change Profile", ButtonStyle::Secondary);
    help = Button({cx - bw * 0.5f, 512.0f, bw, 44.0f}, "How to Play", ButtonStyle::Secondary);
    quit = Button({cx - bw * 0.5f, 568.0f, bw, 44.0f}, "Quit", ButtonStyle::Secondary);

    nameBox = TextBox({cx - 240.0f, 512.0f, 340.0f, 40.0f}, "Type a name, then press Enter");
    nameBox.searchIcon = false;
    createProfile = Button({cx + 112.0f, 512.0f, 128.0f, 40.0f}, "Create", ButtonStyle::Primary);
    backProfiles = Button({cx - 100.0f, 566.0f, 200.0f, 38.0f}, "Back to Menu", ButtonStyle::Secondary);
    backMaps = Button({MAP_LIST_X, 632.0f, 170.0f, 40.0f}, "Back to Menu", ButtonStyle::Secondary);
    playMap = Button({MAP_DETAIL_X + MAP_DETAIL_W - 230.0f, 624.0f, 230.0f, 48.0f}, "Play this map", ButtonStyle::Primary);
    continueMap = Button({MAP_DETAIL_X + MAP_DETAIL_W - 452.0f, 624.0f, 210.0f, 48.0f}, "Continue save", ButtonStyle::Secondary);
    mapSearchBox = TextBox({MAP_LIST_X, 100.0f, MAP_LIST_W, 36.0f}, "Type to search maps...");

    const float pw = 220.0f;
    resume = Button({cx - pw * 0.5f, 300.0f, pw, 44.0f}, "Resume", ButtonStyle::Primary);
    restartPause = Button({cx - pw * 0.5f, 354.0f, pw, 40.0f}, "Restart Game", ButtonStyle::Secondary);
    menuPause = Button({cx - pw * 0.5f, 404.0f, pw, 40.0f}, "Main Menu", ButtonStyle::Secondary);

    restartEnd = Button({cx - 200.0f, 452.0f, 190.0f, 44.0f}, "Restart Game", ButtonStyle::Primary);
    menuEnd = Button({cx + 10.0f, 452.0f, 190.0f, 44.0f}, "Main Menu", ButtonStyle::Secondary);

    closeHelp = Button({cx - 90.0f, 566.0f, 180.0f, 42.0f}, "Got it", ButtonStyle::Primary);
}

void Screens::dim(Gfx& g, int alpha) const {
    g.fillRect(0, 0, WINDOW_W, WINDOW_H, rgba(6, 8, 14, alpha));
}

ScreenAction Screens::menuHit(float mx, float my, bool hasSave) const {
    if (newGame.contains(mx, my)) return ScreenAction::NewGame;
    if (hasSave && continueGame.contains(mx, my)) return ScreenAction::Continue;
    if (changeProfile.contains(mx, my)) return ScreenAction::Profiles;
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

ProfileHit Screens::profilesHit(float mx, float my, int rowCount) const {
    int n = std::min(rowCount, ProfileManager::MAX_PROFILES);
    for (int i = 0; i < n; i++) {
        if (inside(profileRect(i), mx, my)) return {ScreenAction::PickProfile, i};
        if (inside(deleteRect(i), mx, my)) return {ScreenAction::DeleteProfile, i};
    }
    if (createProfile.contains(mx, my)) return {ScreenAction::CreateProfile, -1};
    if (backProfiles.contains(mx, my)) return {ScreenAction::Back, -1};
    return {ScreenAction::None, -1};
}

MapHit Screens::mapSelectHit(float mx, float my, const MapSelectView& view) const {
    for (int i = 0; i < static_cast<int>(view.rows.size()); i++) {
        if (inside(mapRowRect(i), mx, my)) {
            if (i == view.selected) return {ScreenAction::PickMap, i};
            return {ScreenAction::SelectMap, i};
        }
    }
    if (view.selected >= 0 && playMap.contains(mx, my)) return {ScreenAction::PickMap, view.selected};
    if (view.selected >= 0 && view.selected < static_cast<int>(view.rows.size()) && view.rows[view.selected].savedWave > 0 &&
        continueMap.contains(mx, my))
        return {ScreenAction::ContinueMap, view.selected};
    if (mapSearchBox.contains(mx, my)) return {ScreenAction::FocusMapSearch, -1};
    if (backMaps.contains(mx, my)) return {ScreenAction::Back, -1};
    return {ScreenAction::None, -1};
}

void Screens::drawMenu(Gfx& g, const MouseState& mouse, bool hasSave, const std::string& profileName) const {
    const float cx = WINDOW_W * 0.5f;
    g.fillPolygonColors({{0, 0}, {WINDOW_W, 0}, {WINDOW_W, WINDOW_H}, {0, WINDOW_H}},
                        {rgba(8, 10, 18, 150), rgba(8, 10, 18, 150), rgba(8, 10, 18, 235), rgba(8, 10, 18, 235)});
    g.glow(cx, 190.0f, 420.0f, 150.0f, rgba(8, 10, 18, 170));
    g.glow(cx, 175.0f, 300.0f, 70.0f, rgba(234, 186, 78, 26));

    g.textCentered(FontId::Title, "Siege and Supply", cx, 168.0f, pal::gold, true);
    ornament(g, cx, 222.0f, 200.0f);
    g.textCentered(FontId::Heading, "A Tower Defence Strategy Game", cx, 254.0f, rgba(226, 220, 204), true);
    if (!profileName.empty()) g.textCentered(FontId::BodyMedium, "Playing as " + profileName, cx, 300.0f, pal::gold);

    Button cont = continueGame;
    cont.enabled = hasSave;
    newGame.draw(g, mouse);
    cont.draw(g, mouse);
    changeProfile.draw(g, mouse);
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
    panelCard(g, cx - 260.0f, 170.0f, 520.0f, 350.0f);
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
    char buf[96];
    std::snprintf(buf, sizeof(buf), "Score %d", s.score);
    g.textCentered(FontId::Heading, buf, cx, 396.0f, s.newBestScore ? pal::gold : pal::uiText, true);
    std::string line;
    if (s.newBestScore) line = "New best on " + s.mapName + "!";
    else line = "Best on " + s.mapName + ": " + std::to_string(s.bestScore);
    if (s.victory) {
        if (!s.timeKnown) line += "   |   time not counted (old save)";
        else if (s.newBestTime) line += "   |   fastest win " + RecordBook::formatTime(s.timeMs) + "!";
        else line += "   |   time " + RecordBook::formatTime(s.timeMs) + " (best " + RecordBook::formatTime(s.bestTimeMs) + ")";
        line += "   |   castle " + std::to_string(s.castleHealth) + " hp";
    }
    g.textCentered(FontId::Small, line, cx, 418.0f, s.newBestScore || s.newBestTime ? pal::gold : pal::uiMuted);
    restartEnd.draw(g, mouse);
    menuEnd.draw(g, mouse);
}

void Screens::drawHelp(Gfx& g, const MouseState& mouse) const {
    dim(g, 175);
    const float cx = WINDOW_W * 0.5f;
    panelCard(g, cx - 320.0f, 70.0f, 640.0f, 560.0f);
    g.textCentered(FontId::Heading, "How to Play", cx, 108.0f, pal::gold, true);
    ornament(g, cx, 136.0f, 160.0f);
    const char* lines[] = {
        "Enemies crawl out of the purple burrows and march to your castle. Some maps have two burrows.",
        "Pick a tower in the Armory (press 1-4, or type its name in the search box, / for short), then click open grass to build.",
        "Spells: Z Freeze stops enemies, X Mud slows them, C Fire burns them. Press again for tier II and III. Tab switches tabs.",
        "Every enemy finds its own way to the castle. Towers bend their routes, but you can never fully block them.",
        "Raiders attack towers in their orange ring. Press T for a Cannon Cart: it drives over and shoots them.",
        "Ammo never reloads by itself. Select a tower and send an ammo drone (A) or a repair drone (R).",
        "Drones fly a planned route from the castle and steer round Raiders, who can shoot them down.",
        "Click a tower to upgrade it (U) or sell it (S). Space starts the next wave, F changes speed, P pauses, F11 toggles full screen.",
        "Every profile has its own save, plus its best score and fastest win on every map.",
    };
    float y = 160.0f;
    for (const char* line : lines) {
        std::vector<std::string> wrapped = wrapText(g, FontId::Body, line, 540.0f);
        g.fillPolygon({{cx - 282.0f, y + 6.0f}, {cx - 278.0f, y + 10.0f}, {cx - 282.0f, y + 14.0f}, {cx - 286.0f, y + 10.0f}}, pal::gold);
        for (const std::string& w : wrapped) {
            g.text(FontId::Body, w, cx - 268.0f, y, rgba(220, 222, 230));
            y += 20.0f;
        }
        y += 7.0f;
    }
    closeHelp.draw(g, mouse);
}

void Screens::drawMapPreview(Gfx& g, int level, float px, float py, float c) {
    const Theme& th = themeFor(engine_level_theme(level));
    const float boxW = MAP_W * c;
    const float boxH = MAP_H * c;
    const float hw = boxW / (MAP_W + MAP_H);
    const float hh = hw * 0.62f;
    const float depth = hw * 1.6f;
    const float ox = px + MAP_H * hw;
    const float oy = py + (boxH - ((MAP_W + MAP_H) * hh + depth)) * 0.5f;
    auto P = [&](float u, float v, float z) -> SDL_FPoint { return {ox + (u - v) * hw, oy + (u + v) * hh - z}; };
    auto diamond = [&](int x, int y, float z) -> std::vector<SDL_FPoint> {
        return {P(x, y, z), P(x + 1.0f, y, z), P(x + 1.0f, y + 1.0f, z), P(x, y + 1.0f, z)};
    };
    g.glow(px + boxW * 0.5f, oy + (MAP_W + MAP_H) * hh + depth, boxW * 0.5f, hw * 4.0f, rgba(0, 0, 0, 120));
    g.fillPolygonColors({P(0, MAP_H, 0), P(MAP_W, MAP_H, 0), P(MAP_W, MAP_H, -depth), P(0, MAP_H, -depth)},
                        {th.cliffTop, th.cliffTop, th.cliffBottom, th.cliffBottom});
    g.fillPolygonColors({P(MAP_W, MAP_H, 0), P(MAP_W, 0, 0), P(MAP_W, 0, -depth), P(MAP_W, MAP_H, -depth)},
                        {shade(th.cliffTop, 0.72f), shade(th.cliffTop, 0.72f), shade(th.cliffBottom, 0.7f),
                         shade(th.cliffBottom, 0.7f)});
    g.fillPolygon({P(0, 0, 0), P(MAP_W, 0, 0), P(MAP_W, MAP_H, 0), P(0, MAP_H, 0)}, th.previewA);
    const bool detail = hw >= 4.0f;
    for (int y = 0; y < MAP_H; y++) {
        for (int x = 0; x < MAP_W; x++) {
            int t = engine_level_preview(level, x, y);
            if (t == CELL_PATH || t == CELL_SPAWN || t == CELL_GOAL) g.fillPolygon(diamond(x, y, 0.0f), th.previewPath);
            else if (detail && t == CELL_BUILDABLE && (x + y) % 2 == 0) g.fillPolygon(diamond(x, y, 0.0f), th.previewB);
            else if (t == CELL_BLOCKED && engine_level_feature(level, x, y) && th.feature == Feature::LavaPool)
                g.fillPolygon(diamond(x, y, 0.0f), th.previewFeature);
        }
    }
    for (int d = 0; d < MAP_W + MAP_H - 1; d++) {
        for (int x = 0; x < MAP_W; x++) {
            int y = d - x;
            if (y < 0 || y >= MAP_H) continue;
            int t = engine_level_preview(level, x, y);
            SDL_FPoint base = P(x + 0.5f, y + 0.5f, 0.0f);
            if (t == CELL_BLOCKED && engine_level_feature(level, x, y)) {
                if (th.feature == Feature::LavaPool) continue;
                float h = hw * 2.4f;
                g.fillTriangle({base.x - hw * 0.7f, base.y}, {base.x + hw * 0.7f, base.y}, {base.x, base.y - h},
                               shade(th.previewFeature, 0.7f));
                g.fillTriangle({base.x, base.y}, {base.x + hw * 0.7f, base.y}, {base.x, base.y - h}, th.previewFeature);
            } else if (t == CELL_BLOCKED) {
                float h = hw * 1.1f;
                std::vector<SDL_FPoint> top = diamond(x, y, h);
                g.fillPolygon({P(x, y + 1.0f, 0), P(x + 1.0f, y + 1.0f, 0), P(x + 1.0f, y + 1.0f, h), P(x, y + 1.0f, h)}, th.rockMid);
                g.fillPolygon({P(x + 1.0f, y + 1.0f, 0), P(x + 1.0f, y, 0), P(x + 1.0f, y, h), P(x + 1.0f, y + 1.0f, h)},
                              th.rockDark);
                g.fillPolygon(top, th.rockLight);
            } else if (t == CELL_SPAWN) {
                g.glow(base.x, base.y, hw * 2.2f, hh * 2.2f, rgba(170, 90, 240, 170));
                g.fillEllipse(base.x, base.y, hw * 0.75f, hh * 0.75f, rgba(150, 80, 220));
            } else if (t == CELL_GOAL) {
                float h = hw * 1.6f;
                g.fillRect(base.x - hw * 0.8f, base.y - h, hw * 1.6f, h, pal::stoneLight);
                g.fillRect(base.x, base.y - h, hw * 0.8f, h, shade(pal::stoneLight, 0.8f));
                g.fillTriangle({base.x - hw, base.y - h}, {base.x + hw, base.y - h}, {base.x, base.y - h - hw * 1.4f},
                               rgba(70, 90, 160));
            }
        }
    }
}

void Screens::drawMapSelect(Gfx& g, const MouseState& mouse, const MapSelectView& view) const {
    dim(g, 170);
    g.textCentered(FontId::HeadingLarge, "Choose a Map", WINDOW_W * 0.5f, 46.0f, pal::gold, true);
    ornament(g, WINDOW_W * 0.5f, 78.0f, 170.0f);

    mapSearchBox.draw(g, view.search, true, mouse);
    g.text(FontId::Small, "Up / Down to browse, Enter to play, Tab to continue a save", MAP_LIST_X + 4.0f, 142.0f, pal::uiMuted);

    if (view.rows.empty()) {
        g.textCentered(FontId::Body, "No map matches", MAP_LIST_X + MAP_LIST_W * 0.5f, MAP_ROWS_TOP + 40.0f, pal::uiMuted);
        g.textCentered(FontId::BodyBold, "\"" + view.search + "\"", MAP_LIST_X + MAP_LIST_W * 0.5f, MAP_ROWS_TOP + 64.0f,
                       pal::uiText);
    }
    for (int i = 0; i < static_cast<int>(view.rows.size()); i++) {
        const MapRow& row = view.rows[i];
        SDL_FRect r = mapRowRect(i);
        bool hover = inside(r, mouse.x, mouse.y);
        bool sel = i == view.selected;
        const Theme& th = themeFor(engine_level_theme(row.level));
        if (sel) g.glow(r.x + r.w * 0.5f, r.y + r.h * 0.5f, r.w * 0.6f, r.h * 0.7f, withAlpha(pal::gold, 36));
        Color border = sel ? pal::gold : (hover ? rgba(96, 108, 140) : rgba(56, 64, 88));
        g.fillRoundRect(r.x - 1.5f, r.y + 2.5f, r.w + 3.0f, r.h + 3.0f, 12.0f, rgba(10, 12, 18));
        g.fillRoundRect(r.x - 1.5f, r.y - 1.5f, r.w + 3.0f, r.h + 3.0f, 12.0f, border);
        g.fillRoundRectGradient(r.x, r.y, r.w, r.h, 11.0f, sel || hover ? rgba(44, 51, 72) : rgba(32, 38, 54),
                                rgba(20, 24, 35));
        drawMapPreview(g, row.level, r.x + 10.0f, r.y + 10.0f, 4.0f);
        float tx = r.x + 88.0f;
        g.text(FontId::HeadingSmall, engine_level_name(row.level), tx, r.y + 9.0f, sel ? pal::gold : pal::uiText);
        int burrows = 0;
        for (int y = 0; y < MAP_H; y++)
            for (int x = 0; x < MAP_W; x++)
                if (engine_level_preview(row.level, x, y) == CELL_SPAWN) burrows++;
        char buf[64];
        std::snprintf(buf, sizeof(buf), "%s   |   %d burrow%s", th.label, burrows, burrows == 1 ? "" : "s");
        g.text(FontId::Small, buf, tx, r.y + 36.0f, rgba(200, 160, 240));
        if (row.plays == 0) {
            g.text(FontId::Small, "Not played yet", tx, r.y + 56.0f, pal::uiMuted);
        } else {
            std::snprintf(buf, sizeof(buf), "Best %d   |   %s", row.bestScore,
                          row.wins > 0 ? ("Fastest " + RecordBook::formatTime(row.bestTimeMs)).c_str() : "Not won yet");
            g.text(FontId::SmallBold, buf, tx, r.y + 56.0f, row.wins > 0 ? pal::gold : pal::uiText);
        }
        if (row.savedWave > 0) {
            std::snprintf(buf, sizeof(buf), "SAVED  W%d", row.savedWave);
            float tw = 74.0f;
            float bx = r.x + r.w - tw - 10.0f;
            float by = r.y + r.h - 26.0f;
            g.fillRoundRect(bx, by, tw, 18.0f, 9.0f, withAlpha(pal::good, 60));
            g.strokeRoundRect(bx, by, tw, 18.0f, 9.0f, 1.2f, withAlpha(pal::good, 200));
            g.textCentered(FontId::SmallBold, buf, bx + tw * 0.5f, by + 9.0f, pal::uiText);
        }
        if (row.wins > 0) {
            float sx = r.x + r.w - 18.0f;
            float sy = r.y + 18.0f;
            g.fillPolygon({{sx, sy - 8.0f}, {sx + 8.0f, sy}, {sx, sy + 8.0f}, {sx - 8.0f, sy}}, pal::gold);
            g.fillPolygon({{sx, sy - 4.0f}, {sx + 4.0f, sy}, {sx, sy + 4.0f}, {sx - 4.0f, sy}}, rgba(255, 240, 190));
        }
    }

    panelCard(g, MAP_DETAIL_X, 100.0f, MAP_DETAIL_W, 512.0f);
    if (view.selected < 0 || view.selected >= static_cast<int>(view.rows.size())) {
        g.textCentered(FontId::Body, "Pick a map on the left", MAP_DETAIL_X + MAP_DETAIL_W * 0.5f, 340.0f, pal::uiMuted);
        backMaps.draw(g, mouse);
        return;
    }
    const MapRow& row = view.rows[view.selected];
    float px = MAP_DETAIL_X + (MAP_DETAIL_W - MAP_W * BIG_CELL) * 0.5f;
    drawMapPreview(g, row.level, px, 116.0f, BIG_CELL);
    float cx = MAP_DETAIL_X + MAP_DETAIL_W * 0.5f;
    g.textCentered(FontId::Heading, engine_level_name(row.level), cx, 452.0f, pal::gold, true);
    float ly = 476.0f;
    for (const std::string& line : wrapText(g, FontId::Small, engine_level_blurb(row.level), MAP_DETAIL_W - 60.0f)) {
        g.textCentered(FontId::Small, line, cx, ly + 7.0f, rgba(206, 210, 222));
        ly += 16.0f;
    }

    const char* labels[4] = {"Best score", "Fastest win", "Played", "Won"};
    std::string values[4] = {row.plays > 0 ? std::to_string(row.bestScore) : "-", RecordBook::formatTime(row.bestTimeMs),
                             std::to_string(row.plays), std::to_string(row.wins)};
    for (int i = 0; i < 4; i++) {
        float colX = MAP_DETAIL_X + MAP_DETAIL_W * (0.125f + 0.25f * i);
        g.textCentered(FontId::Number, values[i], colX, 548.0f, i < 2 ? pal::gold : pal::uiText);
        g.textCentered(FontId::Small, labels[i], colX, 576.0f, pal::uiMuted);
    }

    backMaps.draw(g, mouse);
    if (row.savedWave > 0) {
        Button cont = continueMap;
        cont.label = "Continue (wave " + std::to_string(row.savedWave) + ")";
        cont.draw(g, mouse, "Tab");
    }
    playMap.draw(g, mouse, "Enter");
}

void Screens::drawProfiles(Gfx& g, const MouseState& mouse, const std::vector<ProfileRow>& rows, const std::string& input,
                           int pendingDeleteId, bool canGoBack) const {
    dim(g, 150);
    const float cx = WINDOW_W * 0.5f;
    panelCard(g, cx - 270.0f, 60.0f, 540.0f, 560.0f);
    g.textCentered(FontId::Heading, "Choose Your Profile", cx, 98.0f, pal::gold, true);
    ornament(g, cx, 126.0f, 170.0f);
    g.textCentered(FontId::Small, "Each profile keeps its own saved game", cx, 148.0f, pal::uiMuted);

    if (rows.empty()) {
        g.textCentered(FontId::Body, "No profiles yet.", cx, 290.0f, pal::uiText);
        g.textCentered(FontId::Small, "Type a name below and press Create to make your first one.", cx, 316.0f,
                       pal::uiMuted);
    }
    int n = std::min(static_cast<int>(rows.size()), ProfileManager::MAX_PROFILES);
    for (int i = 0; i < n; i++) {
        const ProfileRow& row = rows[i];
        Button pick(profileRect(i), row.name, row.lastUsed ? ButtonStyle::Primary : ButtonStyle::Secondary);
        pick.draw(g, mouse, row.detail);
        bool confirm = row.id == pendingDeleteId;
        Button del(deleteRect(i), confirm ? "Confirm" : "Delete", confirm ? ButtonStyle::Danger : ButtonStyle::Secondary);
        del.draw(g, mouse);
    }

    bool full = static_cast<int>(rows.size()) >= ProfileManager::MAX_PROFILES;
    sectionHeading(g, "New Profile", cx - 240.0f, 486.0f, 480.0f);
    TextBox box = nameBox;
    if (full) box.placeholder = "Delete a profile to add a new one";
    box.draw(g, input, !full, mouse);
    Button create = createProfile;
    create.enabled = !full;
    create.draw(g, mouse);

    Button back = backProfiles;
    if (!canGoBack) back.label = "Quit Game";
    back.draw(g, mouse);
}
