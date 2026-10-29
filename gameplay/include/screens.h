#ifndef SCREENS_H
#define SCREENS_H

#include "ui.h"
#include <string>
#include <vector>

enum class ScreenAction {
    None,
    NewGame,
    Continue,
    Help,
    Quit,
    Resume,
    Restart,
    MainMenu,
    CloseHelp,
    Profiles,
    PickProfile,
    DeleteProfile,
    CreateProfile,
    Back
};

struct ProfileRow {
    int id;
    std::string name;
    std::string detail;
    bool lastUsed;
};

struct ProfileHit {
    ScreenAction action;
    int index;
};

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
    ProfileHit profilesHit(float mx, float my, int rowCount) const;

    void drawMenu(Gfx& g, const MouseState& mouse, bool hasSave, const std::string& profileName) const;
    void drawPause(Gfx& g, const MouseState& mouse) const;
    void drawEnd(Gfx& g, const MouseState& mouse, const EndStats& stats) const;
    void drawHelp(Gfx& g, const MouseState& mouse) const;
    void drawProfiles(Gfx& g, const MouseState& mouse, const std::vector<ProfileRow>& rows, const std::string& input,
                      int pendingDeleteId, bool canGoBack) const;

private:
    Button newGame;
    Button continueGame;
    Button changeProfile;
    Button help;
    Button quit;
    Button createProfile;
    Button backProfiles;
    TextBox nameBox;
    Button resume;
    Button restartPause;
    Button menuPause;
    Button restartEnd;
    Button menuEnd;
    Button closeHelp;

    void dim(Gfx& g, int alpha) const;
};

#endif
