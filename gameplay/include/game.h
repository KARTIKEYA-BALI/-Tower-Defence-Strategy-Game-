#ifndef GAME_H
#define GAME_H

#include "drone.h"
#include "effects.h"
#include "enemy.h"
#include "entity.h"
#include "gfx.h"
#include "map_view.h"
#include "profile_manager.h"
#include "record_book.h"
#include "screens.h"
#include "shop_manager.h"
#include "side_panel.h"
#include "spell_book.h"
#include "tower.h"
#include "troop.h"
#include "wave_manager.h"
#include <memory>
#include <string>
#include <vector>

enum class GameState { Profiles, Menu, MapSelect, Playing, Paused, GameOver, Victory };

enum class Placement { Ok, OffMap, NotBuildable, Occupied, BlocksPath, EnemyInWay, NoGold, TroopInWay };

constexpr int TAB_TOWERS = 0;
constexpr int TAB_SPELLS = 1;
constexpr int TAB_TROOPS = 2;

class Game {
public:
    static constexpr int STARTING_GOLD = 150;
    static constexpr int MAX_CASTLE_HEALTH = 100;
    static constexpr int AMMO_DRONE_COST = 15;
    static constexpr int REPAIR_DRONE_COST = 20;
    static constexpr int MAX_DRONES = 3;

    explicit Game(Gfx& gfx);
    ~Game();
    Game(const Game&) = delete;
    Game& operator=(const Game&) = delete;

    bool isRunning() const { return running; }
    void handleEvent(const SDL_Event& e);
    void update(float realDt);
    void draw();

    void addEntity(std::unique_ptr<Entity> e);
    const std::vector<Enemy*>& enemyList() const { return enemies; }
    const std::vector<Tower*>& towerList() const { return towers; }
    const std::vector<Drone*>& droneList() const { return drones; }
    const std::vector<Troop*>& troopList() const { return troops; }
    void damageTroop(Troop& troop, int amount);
    Enemy* findEnemy(int id) const;
    Tower* findTower(int id) const;
    void damageEnemy(Enemy& enemy, int amount);
    void splashDamage(float x, float y, float radius, int damage);
    void damageCastle(int amount, float x, float y);
    void damageTower(Tower& tower, int amount);
    void damageDrone(Drone& drone, int amount);
    void deliverDrone(Drone& drone, Tower& tower);
    Effects& fx() { return effects; }

    void spawnEnemy(EnemyKind kind, float healthScale);
    void requestWaveStart(bool byPlayer);
    void onWaveCleared(int wave, int bonus);

private:
    Gfx& gfx;
    MapView map;
    ShopManager shop;
    WaveManager waves;
    SidePanel panel;
    Screens screens;
    Effects effects;
    ProfileManager profiles;
    SpellBook spells;
    RecordBook records;

    std::vector<std::unique_ptr<Entity>> entities;
    std::vector<std::unique_ptr<Entity>> pending;
    std::vector<Enemy*> enemies;
    std::vector<Tower*> towers;
    std::vector<Drone*> drones;
    std::vector<Troop*> troops;

    GameState state;
    bool running;
    bool showHelp;
    bool hasSave;

    int gold;
    int castleHealth;
    int nextTowerObjectId;
    int placingType;
    int selectedTowerId;
    int mapVersion;
    int enemiesDefeated;
    int towersBuilt;
    int wavesCleared;

    int levelIndex;
    int activeProfile;
    int pendingDeleteId;
    std::string nameInput;
    std::vector<ProfileRow> profileRows;

    std::string search;
    bool searchFocused;
    std::vector<int> searchResults;

    MouseState mouse;
    int hoverCacheCell;
    int hoverCacheVersion;
    bool hoverCacheCanPlace;

    std::string message;
    Color messageColor;
    float messageTimer;
    std::string bannerTitle;
    std::string bannerSub;
    float bannerTimer;
    float castleFlash;
    float shake;
    float clock;
    float smokeTimer;
    bool fastSpeed;

    int tab;
    bool placingTroop;
    int selectedTroopId;
    std::vector<int> spellResults;
    int placingSpell;
    int placingTier;
    float matchTime;
    float playTime;
    bool timeKnown;
    bool recordDone;
    int lastScore;
    RecordResult lastResult;
    MapRecord lastBest;
    std::string mapSearch;
    std::vector<int> mapResults;
    int mapSelected;
    int savedWaves[ProfileManager::MAX_MAP_SAVES];
    bool textInputOn;

    void resetMatch();
    void startNewGame(int level);
    void openMapSelect();
    void refreshViews();
    void mergePending();
    void removeDead();
    int aliveEnemyCount() const;
    int activeDrones() const;
    void sendDrone(DroneKind kind);
    void updateDanger();

    Tower* towerAt(int cx, int cy) const;
    Tower* selectedTower() const;
    bool enemyBlocksCell(int cell) const;
    bool canPlaceCached(int cx, int cy);
    Placement checkPlacement(int cx, int cy, int typeId);
    void tryPlaceTower(int cx, int cy);
    void upgradeSelected();
    void sellSelected();
    void pickType(int typeId);
    void cancelPlacement();
    void onEnemyKilled(Enemy& enemy);

    void setSearchFocus(bool focus);
    void updateSearch();
    void handleSearchKey(SDL_Keycode k);
    void syncTextInput();

    void pickTroop();
    void tryPlaceTroop(int cx, int cy);
    std::string troopProblem(int cx, int cy) const;
    Troop* troopAt(int cell) const;
    Troop* selectedTroop() const;
    int activeTroops() const;
    bool troopBlocksCell(int cell) const;
    void pickSpell(int kind, int tier);
    void cycleSpell(int kind);
    bool castSpell(int cx, int cy);
    void drawZones(bool flames);
    void drawSpellPreview(int cx, int cy, bool ground);
    void drawTip(const std::string& why, float wx, float wy);
    bool mouseCell(int& cx, int& cy) const;
    void drawPlacementGround();
    void drawPlacementGhost();
    void drawSortedObjects();

    void finishMatch(bool won);
    EndStats buildEndStats() const;
    void updateMapSearch();
    void selectMapRow(int row);
    void handleMapSelectKey(SDL_Keycode k);
    void handleMapSelectClick();
    MapSelectView buildMapView() const;

    void saveGame();
    void loadGame(int level);
    void refreshSavedMaps();
    void continueFromMenu();
    bool inMenus() const;
    std::string activeName() const;
    void openProfiles();
    void closeProfiles();
    void refreshProfileRows();
    void selectProfile(int id);
    void submitNewProfile();
    void deleteProfile(int id);
    void cancelDelete();
    void handleTextInput(const char* text);
    void handleProfilesClick();
    void showMessage(const std::string& text, Color color);
    void showBanner(const std::string& title, const std::string& sub);

    void handleMouseDown(int button);
    void handleKey(const SDL_KeyboardEvent& key);
    void handleMapClick(int cx, int cy);
    void handlePanelClick();

    PanelModel buildPanelModel() const;
    void drawBattlefield();
    void drawDanger();
    void drawEntities(int layer) const;
    void drawSelectedRange();
    void drawBanner();
    void drawMessage();
};

#endif
