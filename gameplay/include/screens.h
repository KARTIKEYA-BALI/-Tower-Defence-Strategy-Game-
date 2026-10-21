#ifndef SCREENS_H
#define SCREENS_H

#include "ui.h"
#include <string>

enum class ScreenAction { None, NewGame, Continue, Help, Quit, Resume, Restart, MainMenu, CloseHelp };

struct EndStats {
    bool victory;
    int wavesCleared;
    int totalWaves;
    int towersBuilt;
    int enemiesDefeated;
    int castleHealth;
};

class Screens {
public:
    Screens();

    ScreenAction menuHit(float mx, float my, bool hasSave) const;
    ScreenAction pauseHit(float mx, float my) const;
    ScreenAction endHit(float mx, float my) const;
    ScreenAction helpHit(float mx, float my) const;

    void drawMenu(Gfx& g, const MouseState& mouse, bool hasSave) const;
    void drawPause(Gfx& g, const MouseState& mouse) const;
    void drawEnd(Gfx& g, const MouseState& mouse, const EndStats& stats) const;
    void drawHelp(Gfx& g, const MouseState& mouse) const;

private:
    Button newGame;
    Button continueGame;
    Button help;
    Button quit;
    Button resume;
    Button restartPause;
    Button menuPause;
    Button restartEnd;
    Button menuEnd;
    Button closeHelp;

    void dim(Gfx& g, int alpha) const;
};

#endif
