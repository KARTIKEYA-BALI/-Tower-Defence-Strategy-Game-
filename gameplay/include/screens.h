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
    Back,
    PickMap,
    SelectMap,
    FocusMapSearch,
    ContinueMap
};

struct MapHit {
    ScreenAction action;
    int index;
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
    std::string mapName;
    int score;
    int bestScore;
    bool newBestScore;
    int timeMs;
    bool timeKnown;
    int bestTimeMs;
    bool newBestTime;
};

struct MapRow {
    int level;
    int bestScore;
    int bestTimeMs;
    int plays;
    int wins;
    int savedWave;
};

struct MapSelectView {
    std::vector<MapRow> rows;
    int selected;
    std::string search;
};

class Screens {
public:
    Screens();

    ScreenAction menuHit(float mx, float my, bool hasSave) const;
    ScreenAction pauseHit(float mx, float my) const;
    ScreenAction endHit(float mx, float my) const;
    ScreenAction helpHit(float mx, float my) const;
    ProfileHit profilesHit(float mx, float my, int rowCount) const;
    MapHit mapSelectHit(float mx, float my, const MapSelectView& view) const;

    void drawMenu(Gfx& g, const MouseState& mouse, bool hasSave, const std::string& profileName) const;
    void drawPause(Gfx& g, const MouseState& mouse) const;
    void drawEnd(Gfx& g, const MouseState& mouse, const EndStats& stats) const;
    void drawHelp(Gfx& g, const MouseState& mouse) const;
    void drawProfiles(Gfx& g, const MouseState& mouse, const std::vector<ProfileRow>& rows, const std::string& input,
                      int pendingDeleteId, bool canGoBack) const;
    void drawMapSelect(Gfx& g, const MouseState& mouse, const MapSelectView& view) const;
    static void drawMapPreview(Gfx& g, int level, float px, float py, float cell);

private:
    Button newGame;
    Button continueGame;
    Button changeProfile;
    Button help;
    Button quit;
    Button createProfile;
    Button backProfiles;
    Button backMaps;
    Button playMap;
    Button continueMap;
    TextBox mapSearchBox;
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
