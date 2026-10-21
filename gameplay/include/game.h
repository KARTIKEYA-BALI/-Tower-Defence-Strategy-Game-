#ifndef GAME_H
#define GAME_H

#include "effects.h"
#include "enemy.h"
#include "entity.h"
#include "gfx.h"
#include "map_view.h"
#include "screens.h"
#include "shop_manager.h"
#include "side_panel.h"
#include "tower.h"
#include "wave_manager.h"
#include <memory>
#include <string>
#include <vector>

enum class GameState { Menu, Playing, Paused, GameOver, Victory };

enum class Placement { Ok, OffMap, NotBuildable, Occupied, BlocksPath, EnemyInWay, NoGold };

class Game {
public:
    static constexpr int STARTING_GOLD = 150;
    static constexpr int MAX_CASTLE_HEALTH = 100;

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
    Enemy* findEnemy(int id) const;
    void damageEnemy(Enemy& enemy, int amount);
    void splashDamage(float x, float y, float radius, int damage);
    void damageCastle(int amount, float x, float y);
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

    std::vector<std::unique_ptr<Entity>> entities;
    std::vector<std::unique_ptr<Entity>> pending;
    std::vector<Enemy*> enemies;
    std::vector<Tower*> towers;

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

    void resetMatch();
    void startNewGame();
    void refreshViews();
    void mergePending();
    void removeDead();
    int aliveEnemyCount() const;

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

    void saveGame();
    void loadGame();
    void showMessage(const std::string& text, Color color);
    void showBanner(const std::string& title, const std::string& sub);

    void handleMouseDown(int button);
    void handleKey(const SDL_KeyboardEvent& key);
    void handleMapClick(int cx, int cy);
    void handlePanelClick();

    PanelModel buildPanelModel() const;
    void drawBattlefield();
    void drawEntities(int layer) const;
    void drawPlacementPreview();
    void drawSelectedRange();
    void drawBanner();
    void drawMessage();
};

#endif
