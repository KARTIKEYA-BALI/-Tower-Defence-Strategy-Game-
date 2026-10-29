#include "game.h"
#include "cells.h"
#include "engine_core.h"
#include "projectile.h"
#include "save_manager.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>

namespace {

const char* LEVEL_PATH = "assets/levels/level1.txt";
const float PI_F = 3.14159265358979f;

}

Game::Game(Gfx& g)
    : gfx(g), state(GameState::Profiles), running(true), showHelp(false), hasSave(false), gold(STARTING_GOLD),
      castleHealth(MAX_CASTLE_HEALTH), nextTowerObjectId(0), placingType(-1), selectedTowerId(-1), mapVersion(0),
      enemiesDefeated(0), towersBuilt(0), wavesCleared(0), activeProfile(-1), pendingDeleteId(-1), mouse{0, 0, false},
      hoverCacheCell(-1), hoverCacheVersion(-1), hoverCacheCanPlace(false), messageColor(pal::uiText), messageTimer(0.0f),
      bannerTimer(0.0f), castleFlash(0.0f), shake(0.0f), clock(0.0f), smokeTimer(0.0f), fastSpeed(false) {
    if (!engine_init(MAP_W, MAP_H)) std::fprintf(stderr, "engine_init failed\n");
    std::string level = Gfx::assetPath(LEVEL_PATH);
    if (!engine_load_level(level.c_str())) std::fprintf(stderr, "engine_load_level failed: %s\n", level.c_str());
    profiles.load();
    openProfiles();
}

Game::~Game() {
    entities.clear();
    pending.clear();
    engine_shutdown();
}

void Game::addEntity(std::unique_ptr<Entity> e) {
    pending.push_back(std::move(e));
}

Enemy* Game::findEnemy(int id) const {
    for (Enemy* e : enemies) {
        if (e->getId() == id) return e;
    }
    return nullptr;
}

void Game::refreshViews() {
    enemies.clear();
    towers.clear();
    for (const std::unique_ptr<Entity>& e : entities) {
        if (Enemy* en = dynamic_cast<Enemy*>(e.get())) enemies.push_back(en);
        else if (Tower* t = dynamic_cast<Tower*>(e.get())) towers.push_back(t);
    }
}

void Game::mergePending() {
    if (pending.empty()) return;
    for (std::unique_ptr<Entity>& e : pending) entities.push_back(std::move(e));
    pending.clear();
    refreshViews();
}

void Game::removeDead() {
    size_t before = entities.size();
    entities.erase(std::remove_if(entities.begin(), entities.end(),
                                  [](const std::unique_ptr<Entity>& e) { return !e->isAlive(); }),
                   entities.end());
    if (entities.size() != before) refreshViews();
}

int Game::aliveEnemyCount() const {
    int n = 0;
    for (const Enemy* e : enemies) {
        if (e->isAlive()) n++;
    }
    for (const std::unique_ptr<Entity>& e : pending) {
        if (dynamic_cast<const Enemy*>(e.get())) n++;
    }
    return n;
}

void Game::resetMatch() {
    std::string level = Gfx::assetPath(LEVEL_PATH);
    if (!engine_load_level(level.c_str())) showMessage("Level could not be loaded", pal::danger);
    entities.clear();
    pending.clear();
    refreshViews();
    effects.clear();
    gold = STARTING_GOLD;
    castleHealth = MAX_CASTLE_HEALTH;
    nextTowerObjectId = 0;
    waves.reset();
    placingType = -1;
    selectedTowerId = -1;
    enemiesDefeated = 0;
    towersBuilt = 0;
    wavesCleared = 0;
    fastSpeed = false;
    castleFlash = 0.0f;
    shake = 0.0f;
    bannerTimer = 0.0f;
    messageTimer = 0.0f;
    mapVersion++;
}

void Game::startNewGame() {
    resetMatch();
    state = GameState::Playing;
    showBanner("Prepare your defences", "Build towers, then start wave 1");
}

Tower* Game::towerAt(int cx, int cy) const {
    for (Tower* t : towers) {
        if (t->isAlive() && t->getCellX() == cx && t->getCellY() == cy) return t;
    }
    return nullptr;
}

Tower* Game::selectedTower() const {
    if (selectedTowerId < 0) return nullptr;
    for (Tower* t : towers) {
        if (t->getId() == selectedTowerId && t->isAlive()) return t;
    }
    return nullptr;
}

bool Game::enemyBlocksCell(int cell) const {
    for (const Enemy* e : enemies) {
        if (e->isAlive() && (e->getCell() == cell || e->getTargetCell() == cell)) return true;
    }
    return false;
}

bool Game::canPlaceCached(int cx, int cy) {
    int cell = cellIndex(cx, cy);
    if (cell != hoverCacheCell || mapVersion != hoverCacheVersion) {
        hoverCacheCell = cell;
        hoverCacheVersion = mapVersion;
        hoverCacheCanPlace = engine_can_place_tower(cx, cy) == 1;
    }
    return hoverCacheCanPlace;
}

Placement Game::checkPlacement(int cx, int cy, int typeId) {
    if (!insideMap(cx, cy)) return Placement::OffMap;
    if (towerAt(cx, cy)) return Placement::Occupied;
    if (engine_cell_type(cx, cy) != CELL_BUILDABLE) return Placement::NotBuildable;
    if (!canPlaceCached(cx, cy)) return Placement::BlocksPath;
    if (enemyBlocksCell(cellIndex(cx, cy))) return Placement::EnemyInWay;
    if (gold < shop.type(typeId).cost) return Placement::NoGold;
    return Placement::Ok;
}

void Game::tryPlaceTower(int cx, int cy) {
    if (!shop.validType(placingType)) return;
    const TowerType& type = shop.type(placingType);
    Placement p = checkPlacement(cx, cy, placingType);
    switch (p) {
    case Placement::OffMap:
        return;
    case Placement::Occupied:
        showMessage("A tower already stands there", pal::danger);
        return;
    case Placement::NotBuildable:
        showMessage("Can't build here", pal::danger);
        return;
    case Placement::BlocksPath:
        showMessage("You can't block the path!", pal::danger);
        return;
    case Placement::EnemyInWay:
        showMessage("An enemy is in the way", pal::danger);
        return;
    case Placement::NoGold:
        showMessage("Not enough gold", pal::danger);
        return;
    case Placement::Ok:
        break;
    }

    int objectId = nextTowerObjectId;
    if (!engine_place_tower(cx, cy, objectId)) {
        showMessage("Can't build here", pal::danger);
        mapVersion++;
        return;
    }
    for (const Enemy* e : enemies) {
        if (e->isAlive() && engine_distance_to_goal(e->getTargetCell()) < 0) {
            engine_remove_tower(cx, cy);
            mapVersion++;
            showMessage("That would trap an enemy!", pal::danger);
            return;
        }
    }

    addEntity(std::make_unique<Tower>(objectId, type, cx, cy));
    nextTowerObjectId++;
    gold -= type.cost;
    towersBuilt++;
    mapVersion++;
    float px = cx * CELL_SIZE + CELL_SIZE * 0.5f;
    float py = cy * CELL_SIZE + CELL_SIZE * 0.5f;
    effects.smoke(px, py + 10.0f, 6);
    effects.floatText(px, py - 30.0f, "-" + std::to_string(type.cost), pal::gold);
}

void Game::upgradeSelected() {
    Tower* t = selectedTower();
    if (!t) return;
    if (!t->canUpgrade()) {
        showMessage("Already at max level", pal::uiText);
        return;
    }
    int cost = t->upgradeCost();
    if (gold < cost) {
        showMessage("Not enough gold", pal::danger);
        return;
    }
    gold -= cost;
    t->upgrade();
    effects.explosion(t->getX(), t->getY(), 34.0f, pal::gold);
    effects.floatText(t->getX(), t->getY() - 30.0f, "Level " + std::to_string(t->getLevel()), pal::gold);
}

void Game::sellSelected() {
    Tower* t = selectedTower();
    if (!t) return;
    engine_remove_tower(t->getCellX(), t->getCellY());
    int refund = t->sellValue();
    gold += refund;
    effects.smoke(t->getX(), t->getY(), 10);
    effects.floatText(t->getX(), t->getY() - 30.0f, "+" + std::to_string(refund), pal::gold);
    t->destroy();
    selectedTowerId = -1;
    mapVersion++;
}

void Game::pickType(int typeId) {
    if (!shop.validType(typeId)) return;
    placingType = (placingType == typeId) ? -1 : typeId;
    if (Tower* t = selectedTower()) t->setSelected(false);
    selectedTowerId = -1;
}

void Game::cancelPlacement() {
    placingType = -1;
}

void Game::onEnemyKilled(Enemy& e) {
    gold += e.getReward();
    enemiesDefeated++;
    effects.death(e.getX(), e.getY(), enemyStats(e.getKind()).body, e.getRadius());
    effects.floatText(e.getX(), e.getY() - e.getRadius() - 14.0f, "+" + std::to_string(e.getReward()), pal::gold);
}

void Game::damageEnemy(Enemy& enemy, int amount) {
    if (enemy.takeDamage(amount)) onEnemyKilled(enemy);
}

void Game::splashDamage(float x, float y, float radius, int damage) {
    for (Enemy* e : enemies) {
        if (!e->isAlive()) continue;
        float dx = e->getX() - x;
        float dy = e->getY() - y;
        float d = std::sqrt(dx * dx + dy * dy) - e->getRadius();
        if (d > radius) continue;
        float falloff = 1.0f - 0.5f * std::clamp(d / radius, 0.0f, 1.0f);
        damageEnemy(*e, std::max(1, static_cast<int>(std::lround(damage * falloff))));
    }
}

void Game::damageCastle(int amount, float x, float y) {
    castleHealth = std::max(0, castleHealth - amount);
    castleFlash = 0.35f;
    shake = std::min(9.0f, shake + 2.0f + amount * 0.3f);
    effects.rubble(x, y);
    effects.floatText(x, y - 40.0f, "-" + std::to_string(amount), pal::danger);
}

void Game::spawnEnemy(EnemyKind kind, float healthScale) {
    int s = engine_spawn_cell();
    if (s < 0) return;
    addEntity(std::make_unique<Enemy>(kind, s, healthScale));
    effects.flash(cellCentreX(s), cellCentreY(s), rgba(200, 140, 255));
}

void Game::requestWaveStart(bool byPlayer) {
    if (state != GameState::Playing || !waves.readyToStart()) return;
    int bonus = waves.earlyCallBonus();
    if (byPlayer && bonus > 0) {
        gold += bonus;
        showMessage("Called early: +" + std::to_string(bonus) + " gold", pal::gold);
    }
    if (waves.startNextWave()) {
        char title[32];
        std::snprintf(title, sizeof(title), "Wave %d", waves.waveNumber());
        showBanner(title, waves.waveTitle(waves.waveNumber()));
    }
}

void Game::onWaveCleared(int wave, int bonus) {
    wavesCleared = wave;
    gold += bonus;
    char title[48];
    std::snprintf(title, sizeof(title), "Wave %d cleared", wave);
    showBanner(title, "+" + std::to_string(bonus) + " gold");
}

void Game::showMessage(const std::string& text, Color color) {
    message = text;
    messageColor = color;
    messageTimer = 2.4f;
}

void Game::showBanner(const std::string& title, const std::string& sub) {
    bannerTitle = title;
    bannerSub = sub;
    bannerTimer = 2.6f;
}

bool Game::inMenus() const {
    return state == GameState::Profiles || state == GameState::Menu;
}

std::string Game::activeName() const {
    const Profile* p = profiles.find(activeProfile);
    return p ? p->name : "";
}

void Game::openProfiles() {
    state = GameState::Profiles;
    nameInput.clear();
    pendingDeleteId = -1;
    refreshProfileRows();
    SDL_StartTextInput();
}

void Game::closeProfiles() {
    nameInput.clear();
    cancelDelete();
    SDL_StopTextInput();
    state = GameState::Menu;
}

void Game::refreshProfileRows() {
    profileRows.clear();
    for (const Profile& p : profiles.all()) {
        std::string path = profiles.savePath(p.id);
        SaveData data;
        std::string error;
        std::string detail = "No saved game yet";
        if (SaveManager::load(path, data, error)) detail = "Saved game at wave " + std::to_string(data.wave);
        else if (SaveManager::exists(path)) detail = "Save file is damaged";
        bool last = p.id == profiles.lastUsed();
        if (last) detail = "Last played  |  " + detail;
        profileRows.push_back({p.id, p.name, detail, last});
    }
}

void Game::selectProfile(int id) {
    const Profile* p = profiles.find(id);
    if (!p) return;
    activeProfile = id;
    profiles.setLastUsed(id);
    hasSave = SaveManager::exists(profiles.savePath(id));
    closeProfiles();
    showMessage("Welcome, " + p->name, pal::good);
}

void Game::submitNewProfile() {
    int id = -1;
    switch (profiles.create(nameInput, id)) {
    case ProfileResult::Ok:
        selectProfile(id);
        break;
    case ProfileResult::EmptyName:
        showMessage("Type a name first", pal::danger);
        break;
    case ProfileResult::NameTaken:
        showMessage("That name is already taken", pal::danger);
        break;
    case ProfileResult::Full:
        showMessage("Only " + std::to_string(ProfileManager::MAX_PROFILES) + " profiles allowed", pal::danger);
        break;
    case ProfileResult::WriteFailed:
        showMessage("Could not save the profile list", pal::danger);
        break;
    }
}

void Game::deleteProfile(int id) {
    const Profile* p = profiles.find(id);
    if (!p) return;
    std::string name = p->name;
    pendingDeleteId = -1;
    if (!profiles.remove(id)) {
        showMessage("Could not delete " + name, pal::danger);
        return;
    }
    if (activeProfile == id) {
        activeProfile = -1;
        hasSave = false;
    }
    refreshProfileRows();
    showMessage("Deleted " + name, pal::uiText);
}

void Game::cancelDelete() {
    if (pendingDeleteId < 0) return;
    pendingDeleteId = -1;
    messageTimer = 0.0f;
}

void Game::handleTextInput(const char* text) {
    if (state != GameState::Profiles || showHelp) return;
    for (const char* c = text; *c; c++) {
        if (!ProfileManager::validChar(*c) || nameInput.size() >= ProfileManager::MAX_NAME) continue;
        if (*c == ' ' && (nameInput.empty() || nameInput.back() == ' ')) continue;
        nameInput.push_back(*c);
    }
}

void Game::handleProfilesClick() {
    ProfileHit hit = screens.profilesHit(mouse.x, mouse.y, static_cast<int>(profileRows.size()));
    if (hit.action != ScreenAction::DeleteProfile) cancelDelete();
    switch (hit.action) {
    case ScreenAction::PickProfile:
        selectProfile(profileRows[hit.index].id);
        break;
    case ScreenAction::DeleteProfile: {
        const ProfileRow& row = profileRows[hit.index];
        if (pendingDeleteId == row.id) {
            deleteProfile(row.id);
        } else {
            pendingDeleteId = row.id;
            showMessage("Click Confirm to delete " + row.name + " and its save", pal::danger);
        }
        break;
    }
    case ScreenAction::CreateProfile:
        submitNewProfile();
        break;
    case ScreenAction::Back:
        if (activeProfile >= 0) closeProfiles();
        else running = false;
        break;
    default:
        break;
    }
}

void Game::saveGame() {
    if (activeProfile < 0) {
        showMessage("Choose a profile first", pal::danger);
        return;
    }
    SaveData data;
    data.gold = gold;
    data.health = castleHealth;
    data.wave = waves.waveNumber();
    for (const Tower* t : towers) {
        if (!t->isAlive()) continue;
        data.towers.push_back({t->getObjectId(), t->getTypeId(), t->getCellX(), t->getCellY(), t->getLevel()});
    }
    std::size_t raw = 0;
    std::size_t stored = 0;
    std::string error;
    if (SaveManager::save(data, profiles.savePath(activeProfile), raw, stored, error)) {
        hasSave = true;
        showMessage("Saved for " + activeName() + ": " + std::to_string(raw) + " -> " + std::to_string(stored) + " bytes",
                    pal::good);
    } else {
        showMessage(error, pal::danger);
    }
}

void Game::loadGame() {
    if (activeProfile < 0) {
        showMessage("Choose a profile first", pal::danger);
        return;
    }
    SaveData data;
    std::string error;
    if (!SaveManager::load(profiles.savePath(activeProfile), data, error)) {
        showMessage(error, pal::danger);
        return;
    }
    resetMatch();
    gold = std::max(0, data.gold);
    castleHealth = std::clamp(data.health, 1, MAX_CASTLE_HEALTH);
    int maxId = -1;
    int skipped = 0;
    for (const SavedTower& st : data.towers) {
        if (!shop.validType(st.typeId) || st.objectId < 0 || !engine_place_tower(st.cellX, st.cellY, st.objectId)) {
            skipped++;
            continue;
        }
        std::unique_ptr<Tower> t = std::make_unique<Tower>(st.objectId, shop.type(st.typeId), st.cellX, st.cellY);
        t->setLevel(st.level);
        addEntity(std::move(t));
        maxId = std::max(maxId, st.objectId);
    }
    mergePending();
    nextTowerObjectId = maxId + 1;
    towersBuilt = static_cast<int>(towers.size());
    waves.restartAt(data.wave);
    wavesCleared = waves.waveNumber() - 1;
    mapVersion++;
    state = GameState::Playing;
    char sub[64];
    if (skipped > 0) std::snprintf(sub, sizeof(sub), "%d tower(s) could not be restored", skipped);
    else std::snprintf(sub, sizeof(sub), "Wave %d is ready when you are", waves.waveNumber());
    showBanner("Game loaded", sub);
}

void Game::handleEvent(const SDL_Event& e) {
    int mx = 0;
    int my = 0;
    SDL_GetMouseState(&mx, &my);
    mouse.x = static_cast<float>(mx);
    mouse.y = static_cast<float>(my);

    switch (e.type) {
    case SDL_QUIT:
        running = false;
        break;
    case SDL_MOUSEBUTTONDOWN:
        if (e.button.button == SDL_BUTTON_LEFT) mouse.down = true;
        handleMouseDown(e.button.button);
        break;
    case SDL_MOUSEBUTTONUP:
        if (e.button.button == SDL_BUTTON_LEFT) mouse.down = false;
        break;
    case SDL_KEYDOWN:
        handleKey(e.key);
        break;
    case SDL_TEXTINPUT:
        handleTextInput(e.text.text);
        break;
    case SDL_WINDOWEVENT:
        if (e.window.event == SDL_WINDOWEVENT_FOCUS_LOST && state == GameState::Playing) {
            mouse.down = false;
        }
        break;
    default:
        break;
    }
}

void Game::handleKey(const SDL_KeyboardEvent& key) {
    SDL_Keycode k = key.keysym.sym;

    if (showHelp) {
        if (k == SDLK_ESCAPE || k == SDLK_RETURN || k == SDLK_SPACE) showHelp = false;
        return;
    }

    if (state == GameState::Profiles) {
        if (k == SDLK_BACKSPACE) {
            if (!nameInput.empty()) nameInput.pop_back();
        } else if (k == SDLK_RETURN || k == SDLK_KP_ENTER) {
            if (!nameInput.empty()) submitNewProfile();
            else if (profiles.find(profiles.lastUsed())) selectProfile(profiles.lastUsed());
            else showMessage("Type a name first", pal::danger);
        } else if (k == SDLK_ESCAPE) {
            if (!nameInput.empty()) nameInput.clear();
            else if (activeProfile >= 0) closeProfiles();
            else running = false;
        }
        return;
    }
    if (state == GameState::Menu) {
        if (k == SDLK_RETURN || k == SDLK_SPACE) startNewGame();
        else if (k == SDLK_ESCAPE) running = false;
        return;
    }
    if (state == GameState::Paused) {
        if (k == SDLK_ESCAPE || k == SDLK_p) state = GameState::Playing;
        return;
    }
    if (state == GameState::GameOver || state == GameState::Victory) {
        if (k == SDLK_RETURN) startNewGame();
        return;
    }

    switch (k) {
    case SDLK_1:
    case SDLK_2:
    case SDLK_3:
    case SDLK_4:
        pickType(static_cast<int>(k - SDLK_1));
        break;
    case SDLK_ESCAPE:
        if (placingType >= 0) cancelPlacement();
        else if (selectedTowerId >= 0) selectedTowerId = -1;
        else state = GameState::Paused;
        break;
    case SDLK_p:
        state = GameState::Paused;
        break;
    case SDLK_SPACE:
        requestWaveStart(true);
        break;
    case SDLK_u:
        upgradeSelected();
        break;
    case SDLK_s:
    case SDLK_DELETE:
        sellSelected();
        break;
    case SDLK_f:
        fastSpeed = !fastSpeed;
        break;
    default:
        break;
    }
}

void Game::handleMouseDown(int button) {
    if (showHelp) {
        if (button == SDL_BUTTON_LEFT && screens.helpHit(mouse.x, mouse.y) == ScreenAction::CloseHelp) showHelp = false;
        return;
    }
    if (button != SDL_BUTTON_LEFT && button != SDL_BUTTON_RIGHT) return;

    if (state == GameState::Profiles) {
        if (button == SDL_BUTTON_LEFT) handleProfilesClick();
        return;
    }
    if (state == GameState::Menu) {
        if (button != SDL_BUTTON_LEFT) return;
        switch (screens.menuHit(mouse.x, mouse.y, hasSave)) {
        case ScreenAction::NewGame:
            startNewGame();
            break;
        case ScreenAction::Continue:
            loadGame();
            break;
        case ScreenAction::Profiles:
            openProfiles();
            break;
        case ScreenAction::Help:
            showHelp = true;
            break;
        case ScreenAction::Quit:
            running = false;
            break;
        default:
            break;
        }
        return;
    }
    if (state == GameState::Paused) {
        if (button != SDL_BUTTON_LEFT) return;
        switch (screens.pauseHit(mouse.x, mouse.y)) {
        case ScreenAction::Resume:
            state = GameState::Playing;
            break;
        case ScreenAction::Restart:
            startNewGame();
            break;
        case ScreenAction::MainMenu:
            resetMatch();
            state = GameState::Menu;
            break;
        default:
            break;
        }
        return;
    }
    if (state == GameState::GameOver || state == GameState::Victory) {
        if (button != SDL_BUTTON_LEFT) return;
        ScreenAction a = screens.endHit(mouse.x, mouse.y);
        if (a == ScreenAction::Restart) startNewGame();
        else if (a == ScreenAction::MainMenu) {
            resetMatch();
            state = GameState::Menu;
        }
        return;
    }

    if (button == SDL_BUTTON_RIGHT) {
        if (placingType >= 0) cancelPlacement();
        else selectedTowerId = -1;
        return;
    }

    int px = static_cast<int>(mouse.x);
    int py = static_cast<int>(mouse.y);
    if (pointOnMap(px, py)) {
        handleMapClick(px / CELL_SIZE, py / CELL_SIZE);
    } else {
        handlePanelClick();
    }
}

void Game::handleMapClick(int cx, int cy) {
    if (placingType >= 0) {
        if (Tower* t = towerAt(cx, cy)) {
            placingType = -1;
            selectedTowerId = t->getId();
            return;
        }
        tryPlaceTower(cx, cy);
        return;
    }
    Tower* t = towerAt(cx, cy);
    selectedTowerId = t ? t->getId() : -1;
}

void Game::handlePanelClick() {
    PanelModel model = buildPanelModel();
    PanelHit hit = panel.hitTest(mouse.x, mouse.y, model);
    switch (hit.action) {
    case PanelAction::StartWave:
        requestWaveStart(true);
        break;
    case PanelAction::PickType:
        pickType(hit.typeId);
        break;
    case PanelAction::Upgrade:
        upgradeSelected();
        break;
    case PanelAction::Sell:
        sellSelected();
        break;
    case PanelAction::Save:
        saveGame();
        break;
    case PanelAction::Load:
        loadGame();
        break;
    case PanelAction::Speed:
        fastSpeed = !fastSpeed;
        break;
    case PanelAction::Pause:
        state = GameState::Paused;
        break;
    case PanelAction::None:
        break;
    }
}

void Game::update(float realDt) {
    realDt = std::min(realDt, 0.05f);
    clock += realDt;
    messageTimer = std::max(0.0f, messageTimer - realDt);
    bannerTimer = std::max(0.0f, bannerTimer - realDt);

    if (state != GameState::Playing || showHelp) return;

    const float dt = realDt * (fastSpeed ? 2.0f : 1.0f);
    castleFlash = std::max(0.0f, castleFlash - dt);
    shake = std::max(0.0f, shake - dt * 30.0f);

    waves.update(dt, *this, aliveEnemyCount());
    for (std::unique_ptr<Entity>& e : entities) {
        if (e->isAlive()) e->update(dt, *this);
    }
    mergePending();
    removeDead();
    effects.update(dt);

    for (Tower* t : towers) t->setSelected(t->getId() == selectedTowerId);
    if (selectedTowerId >= 0 && !selectedTower()) selectedTowerId = -1;

    if (castleHealth < MAX_CASTLE_HEALTH * 0.4f && map.goalCell() >= 0) {
        smokeTimer -= dt;
        if (smokeTimer <= 0.0f) {
            smokeTimer = castleHealth < MAX_CASTLE_HEALTH * 0.2f ? 0.12f : 0.3f;
            effects.smoke(cellCentreX(map.goalCell()) + (std::rand() % 40 - 20), cellCentreY(map.goalCell()) - 30.0f, 1);
        }
    }

    if (castleHealth <= 0) {
        state = GameState::GameOver;
        placingType = -1;
    } else if (waves.allWavesDone() && aliveEnemyCount() == 0) {
        state = GameState::Victory;
        placingType = -1;
    }
}

PanelModel Game::buildPanelModel() const {
    PanelModel m;
    m.health = castleHealth;
    m.maxHealth = MAX_CASTLE_HEALTH;
    m.gold = gold;
    m.wave = waves.waveNumber();
    m.totalWaves = waves.totalWaves();
    m.waveActive = waves.inProgress();
    m.allWavesDone = waves.allWavesDone();
    m.countdown = waves.countdown();
    m.earlyBonus = waves.earlyCallBonus();
    m.enemiesLeft = aliveEnemyCount() + waves.remainingToSpawn();
    m.results = shop.allTypes();
    m.placingType = placingType;
    m.shop = &shop;
    m.selected = selectedTower();
    m.fastSpeed = fastSpeed;
    return m;
}

void Game::drawEntities(int layer) const {
    for (const std::unique_ptr<Entity>& e : entities) {
        if (e->isAlive() && e->layer() == layer) e->draw(gfx);
    }
}

void Game::drawSelectedRange() {
    Tower* t = selectedTower();
    if (!t) return;
    gfx.fillCircle(t->getX(), t->getY(), t->range(), rgba(234, 186, 78, 22));
    gfx.strokeCircle(t->getX(), t->getY(), t->range(), 1.6f, rgba(234, 186, 78, 150));
}

void Game::drawPlacementPreview() {
    int px = static_cast<int>(mouse.x);
    int py = static_cast<int>(mouse.y);
    if (!pointOnMap(px, py)) return;
    int cx = px / CELL_SIZE;
    int cy = py / CELL_SIZE;
    float x = cx * CELL_SIZE;
    float y = cy * CELL_SIZE;
    float mxc = x + CELL_SIZE * 0.5f;
    float myc = y + CELL_SIZE * 0.5f;

    if (placingType < 0) {
        if (towerAt(cx, cy) && !selectedTower()) {
            gfx.strokeCircle(mxc, myc, 23.0f, 1.5f, rgba(255, 255, 255, 90));
        }
        return;
    }

    Placement p = checkPlacement(cx, cy, placingType);
    bool ok = p == Placement::Ok;
    Color c = ok ? rgba(110, 220, 120) : rgba(236, 86, 72);
    gfx.fillRoundRect(x + 2.0f, y + 2.0f, CELL_SIZE - 4.0f, CELL_SIZE - 4.0f, 7.0f, withAlpha(c, 55));
    gfx.strokeRoundRect(x + 2.0f, y + 2.0f, CELL_SIZE - 4.0f, CELL_SIZE - 4.0f, 7.0f, 2.0f, withAlpha(c, 210));

    if (p == Placement::Occupied || p == Placement::OffMap) return;
    float range = shop.type(placingType).range;
    gfx.fillCircle(mxc, myc, range, withAlpha(c, 18));
    gfx.strokeCircle(mxc, myc, range, 1.4f, withAlpha(c, 120));
    float prev = gfx.alpha();
    gfx.setAlpha(ok ? 0.75f : 0.45f);
    Tower::drawModel(gfx, placingType, mxc, myc, 1.0f, -PI_F * 0.5f, 1, 0.0f, true);
    gfx.setAlpha(prev);

    if (!ok && p != Placement::NotBuildable) {
        const char* why = "";
        if (p == Placement::BlocksPath) why = "Would block the path";
        else if (p == Placement::EnemyInWay) why = "Enemy in the way";
        else if (p == Placement::NoGold) why = "Not enough gold";
        float w = gfx.textWidth(FontId::SmallBold, why) + 16.0f;
        float tx = std::clamp(mxc, w * 0.5f + 4.0f, PANEL_X - w * 0.5f - 4.0f);
        float ty = y - 16.0f < 4.0f ? y + CELL_SIZE + 4.0f : y - 20.0f;
        gfx.fillRoundRect(tx - w * 0.5f, ty, w, 18.0f, 9.0f, rgba(20, 10, 10, 210));
        gfx.textCentered(FontId::SmallBold, why, tx, ty + 9.0f, rgba(255, 170, 160));
    }
}

void Game::drawBanner() {
    if (bannerTimer <= 0.0f) return;
    const float total = 2.6f;
    float t = total - bannerTimer;
    float a = std::min(1.0f, t / 0.3f) * std::min(1.0f, bannerTimer / 0.6f);
    float cx = PANEL_X * 0.5f;
    float cy = 250.0f;
    Color band = rgba(8, 10, 16, static_cast<int>(165 * a));
    Color none = rgba(8, 10, 16, 0);
    gfx.fillPolygonColors({{0, cy - 48.0f}, {cx, cy - 48.0f}, {cx, cy + 48.0f}, {0, cy + 48.0f}}, {none, band, band, none});
    gfx.fillPolygonColors({{cx, cy - 48.0f}, {PANEL_X, cy - 48.0f}, {PANEL_X, cy + 48.0f}, {cx, cy + 48.0f}}, {band, none, none, band});
    float rise = (1.0f - std::min(1.0f, t / 0.3f)) * 10.0f;
    gfx.textCentered(FontId::HeadingLarge, bannerTitle, cx, cy - 10.0f + rise, withAlpha(pal::gold, static_cast<int>(255 * a)), true);
    gfx.textCentered(FontId::BodyMedium, bannerSub, cx, cy + 28.0f + rise, withAlpha(pal::uiText, static_cast<int>(230 * a)));
}

void Game::drawMessage() {
    if (messageTimer <= 0.0f || message.empty()) return;
    float a = std::min(1.0f, messageTimer / 0.4f);
    float w = gfx.textWidth(FontId::BodyMedium, message) + 44.0f;
    float cx = inMenus() ? WINDOW_W * 0.5f : PANEL_X * 0.5f;
    float y = MAP_H * CELL_SIZE - 62.0f;
    float prev = gfx.alpha();
    gfx.setAlpha(a);
    gfx.glow(cx, y + 18.0f, w * 0.6f, 26.0f, rgba(0, 0, 0, 140));
    gfx.fillRoundRect(cx - w * 0.5f, y, w, 36.0f, 18.0f, rgba(14, 16, 24, 235));
    gfx.strokeRoundRect(cx - w * 0.5f, y, w, 36.0f, 18.0f, 1.2f, withAlpha(messageColor, 160));
    gfx.fillCircle(cx - w * 0.5f + 17.0f, y + 18.0f, 4.0f, messageColor);
    gfx.textCentered(FontId::BodyMedium, message, cx + 6.0f, y + 18.5f, pal::uiText);
    gfx.setAlpha(prev);
}

void Game::drawBattlefield() {
    float sx = 0.0f;
    float sy = 0.0f;
    if (shake > 0.0f) {
        sx = (static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX) - 0.5f) * shake;
        sy = (static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX) - 0.5f) * shake;
    }
    gfx.setClip(0, 0, PANEL_X, MAP_H * CELL_SIZE);
    gfx.setOffset(sx, sy);

    map.drawGround(gfx);
    if (state == GameState::Playing && placingType >= 0) map.drawGrid(gfx, 1.0f);
    map.drawRoute(gfx);
    map.drawRocks(gfx);
    map.drawPortal(gfx);
    drawEntities(0);
    if (!inMenus()) drawSelectedRange();
    drawEntities(1);
    map.drawCastle(gfx, castleFlash);
    drawEntities(2);
    effects.draw(gfx);
    if (state == GameState::Playing && !showHelp) drawPlacementPreview();
    gfx.setOffset(0.0f, 0.0f);
    map.drawVignette(gfx);
    if (!inMenus()) {
        drawBanner();
        drawMessage();
    }
    gfx.clearClip();
}

void Game::draw() {
    gfx.beginFrame(clock);
    map.syncWithEngine(gfx);
    gfx.clear(pal::uiBg);

    drawBattlefield();

    if (state == GameState::Profiles) {
        screens.drawProfiles(gfx, mouse, profileRows, nameInput, pendingDeleteId, activeProfile >= 0);
    } else if (state == GameState::Menu) {
        screens.drawMenu(gfx, mouse, hasSave, activeName());
    } else {
        panel.draw(gfx, buildPanelModel(), mouse);
        if (state == GameState::Paused) screens.drawPause(gfx, mouse);
        else if (state == GameState::GameOver || state == GameState::Victory) {
            EndStats s = {state == GameState::Victory, wavesCleared, waves.totalWaves(), towersBuilt, enemiesDefeated,
                          castleHealth};
            screens.drawEnd(gfx, mouse, s);
        }
    }
    if (showHelp) screens.drawHelp(gfx, mouse);
    if (inMenus() && messageTimer > 0.0f) drawMessage();

    gfx.endFrame();
}
