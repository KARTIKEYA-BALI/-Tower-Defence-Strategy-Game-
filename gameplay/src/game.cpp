#include "game.h"
#include "cells.h"
#include "engine_core.h"
#include "projectile.h"
#include "save_manager.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cctype>
#include <cstdlib>

namespace {

const float PI_F = 3.14159265358979f;

}

Game::Game(Gfx& g)
    : gfx(g), state(GameState::Profiles), running(true), showHelp(false), hasSave(false), gold(STARTING_GOLD),
      castleHealth(MAX_CASTLE_HEALTH), nextTowerObjectId(0), placingType(-1), selectedTowerId(-1), mapVersion(0),
      enemiesDefeated(0), towersBuilt(0), wavesCleared(0), levelIndex(0), activeProfile(-1), pendingDeleteId(-1),
      searchFocused(false), mouse{0, 0, false}, hoverCacheCell(-1), hoverCacheVersion(-1), hoverCacheCanPlace(false),
      messageColor(pal::uiText), messageTimer(0.0f), bannerTimer(0.0f), castleFlash(0.0f), shake(0.0f), clock(0.0f),
      smokeTimer(0.0f), fastSpeed(false), tab(TAB_TOWERS), placingTroop(false), selectedTroopId(-1), placingSpell(-1), placingTier(0), matchTime(0.0f),
      playTime(0.0f), timeKnown(true), recordDone(false), lastScore(0), lastResult{false, false}, lastBest{-1, -1, 0, 0},
      mapSelected(0), textInputOn(SDL_IsTextInputActive() == SDL_TRUE) {
    for (int& w : savedWaves) w = 0;
    if (!engine_init(MAP_W, MAP_H)) std::fprintf(stderr, "engine_init failed\n");
    if (!shop.registerWithEngine()) std::fprintf(stderr, "shop registration failed\n");
    for (int k = 0; k < spells.count(); k++) {
        if (!shop.addSearchName(spells.type(k).name, SpellBook::SEARCH_ID_BASE + k, 5)) {
            std::fprintf(stderr, "spell registration failed\n");
        }
    }
    updateSearch();
    profiles.load();
    openProfiles();
}

Game::~Game() {
    entities.clear();
    pending.clear();
    shop.shutdownEngineShop();
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

Tower* Game::findTower(int id) const {
    for (Tower* t : towers) {
        if (t->getId() == id && t->isAlive()) return t;
    }
    return nullptr;
}

void Game::refreshViews() {
    enemies.clear();
    towers.clear();
    drones.clear();
    troops.clear();
    for (const std::unique_ptr<Entity>& e : entities) {
        if (Enemy* en = dynamic_cast<Enemy*>(e.get())) enemies.push_back(en);
        else if (Tower* t = dynamic_cast<Tower*>(e.get())) towers.push_back(t);
        else if (Drone* d = dynamic_cast<Drone*>(e.get())) drones.push_back(d);
        else if (Troop* tr = dynamic_cast<Troop*>(e.get())) troops.push_back(tr);
    }
}

int Game::activeTroops() const {
    int n = 0;
    for (const Troop* t : troops) {
        if (t->isAlive()) n++;
    }
    for (const std::unique_ptr<Entity>& e : pending) {
        if (dynamic_cast<const Troop*>(e.get())) n++;
    }
    return n;
}

Troop* Game::troopAt(int cell) const {
    for (Troop* t : troops) {
        if (t->isAlive() && (t->positionCell() == cell || t->getCell() == cell)) return t;
    }
    return nullptr;
}

Troop* Game::selectedTroop() const {
    if (selectedTroopId < 0) return nullptr;
    for (Troop* t : troops) {
        if (t->getId() == selectedTroopId && t->isAlive()) return t;
    }
    return nullptr;
}

bool Game::troopBlocksCell(int cell) const {
    for (const Troop* t : troops) {
        if (!t->isAlive()) continue;
        if (t->getCell() == cell || t->nextCell() == cell || t->positionCell() == cell || t->getPost() == cell) return true;
    }
    return false;
}

void Game::pickTroop() {
    placingTroop = !placingTroop;
    if (placingTroop) {
        placingType = -1;
        placingSpell = -1;
        tab = TAB_TROOPS;
        if (Tower* t = selectedTower()) t->setSelected(false);
        selectedTowerId = -1;
        selectedTroopId = -1;
    }
}

std::string Game::troopProblem(int cx, int cy) const {
    if (!insideMap(cx, cy)) return "Off the map";
    int type = engine_cell_type(cx, cy);
    if (activeTroops() >= Troop::MAX_TROOPS) return "Only " + std::to_string(Troop::MAX_TROOPS) + " carts at a time";
    if ((type != CELL_BUILDABLE && type != CELL_PATH) || towerAt(cx, cy)) return "Needs open ground";
    if (troopBlocksCell(cellIndex(cx, cy))) return "Another cart is there";
    if (gold < Troop::COST) return "Not enough gold";
    return "";
}

void Game::tryPlaceTroop(int cx, int cy) {
    if (!insideMap(cx, cy)) return;
    int cell = cellIndex(cx, cy);
    std::string why = troopProblem(cx, cy);
    if (!why.empty()) {
        showMessage(why, pal::danger);
        return;
    }
    gold -= Troop::COST;
    addEntity(std::make_unique<Troop>(cell));
    placingTroop = false;
    float px = cellCentreX(cell);
    float py = cellCentreY(cell);
    effects.smoke(px, py, 6);
    effects.floatText(px, py - 30.0f, "-" + std::to_string(Troop::COST), pal::gold);
}

void Game::damageTroop(Troop& t, int amount) {
    if (!t.isAlive()) return;
    effects.sparks(t.getX(), t.getY(), rgba(255, 170, 80));
    if (!t.takeDamage(amount)) return;
    effects.explosion(t.getX(), t.getY(), 36.0f, rgba(255, 140, 60));
    effects.rubble(t.getX(), t.getY());
    if (selectedTroopId == t.getId()) selectedTroopId = -1;
    showMessage("Raiders destroyed a Cannon Cart!", pal::danger);
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

int Game::activeDrones() const {
    int n = 0;
    for (const Drone* d : drones) {
        if (d->isAlive()) n++;
    }
    for (const std::unique_ptr<Entity>& e : pending) {
        if (dynamic_cast<const Drone*>(e.get())) n++;
    }
    return n;
}

void Game::resetMatch() {
    if (!engine_load_level_index(levelIndex)) showMessage("Level could not be loaded", pal::danger);
    engine_clear_danger();
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
    spells.reset();
    engine_zone_clear();
    placingSpell = -1;
    placingTroop = false;
    selectedTroopId = -1;
    placingTier = 0;
    matchTime = 0.0f;
    playTime = 0.0f;
    timeKnown = true;
    recordDone = false;
    search.clear();
    setSearchFocus(false);
    updateSearch();
}

void Game::startNewGame(int level) {
    levelIndex = std::clamp(level, 0, engine_level_count() - 1);
    resetMatch();
    state = GameState::Playing;
    showBanner("Prepare your defences", std::string(engine_level_name(levelIndex)) + "  -  build towers, then start wave 1");
}

void Game::refreshSavedMaps() {
    hasSave = false;
    for (int level = 0; level < ProfileManager::MAX_MAP_SAVES; level++) {
        savedWaves[level] = 0;
        if (activeProfile < 0 || level >= engine_level_count()) continue;
        SaveData data;
        std::string error;
        if (SaveManager::load(profiles.mapSavePath(activeProfile, level), data, error)) {
            savedWaves[level] = std::max(1, data.wave);
            hasSave = true;
        }
    }
}

void Game::continueFromMenu() {
    openMapSelect();
    for (int i = 0; i < static_cast<int>(mapResults.size()); i++) {
        int level = mapResults[i];
        if (level >= 0 && level < ProfileManager::MAX_MAP_SAVES && savedWaves[level] > 0) {
            selectMapRow(i);
            return;
        }
    }
}

void Game::openMapSelect() {
    refreshSavedMaps();
    state = GameState::MapSelect;
    mapSearch.clear();
    mapSelected = 0;
    updateMapSearch();
    for (int i = 0; i < static_cast<int>(mapResults.size()); i++) {
        if (mapResults[i] == levelIndex) mapSelected = i;
    }
    selectMapRow(mapSelected);
}

void Game::updateMapSearch() {
    int previous = (mapSelected >= 0 && mapSelected < static_cast<int>(mapResults.size())) ? mapResults[mapSelected] : -1;
    int ids[16];
    int n = map_search(mapSearch.c_str(), ids, 16);
    mapResults.assign(ids, ids + std::max(0, n));
    mapSelected = mapResults.empty() ? -1 : 0;
    for (int i = 0; i < static_cast<int>(mapResults.size()); i++) {
        if (mapResults[i] == previous) mapSelected = i;
    }
    selectMapRow(mapSelected);
}

void Game::selectMapRow(int row) {
    if (row < 0 || row >= static_cast<int>(mapResults.size())) return;
    mapSelected = row;
    int level = mapResults[row];
    if (engine_current_level() != level && entities.empty()) {
        levelIndex = level;
        engine_load_level_index(level);
        mapVersion++;
    }
}

MapSelectView Game::buildMapView() const {
    MapSelectView v;
    v.selected = mapSelected;
    v.search = mapSearch;
    for (int level : mapResults) {
        MapRecord r = records.get(RecordBook::mapKey(engine_level_name(level)));
        int saved = (level >= 0 && level < ProfileManager::MAX_MAP_SAVES) ? savedWaves[level] : 0;
        v.rows.push_back({level, r.bestScore, r.bestTimeMs, r.plays, r.wins, saved});
    }
    return v;
}

void Game::handleMapSelectKey(SDL_Keycode k) {
    int count = static_cast<int>(mapResults.size());
    if (k == SDLK_UP || k == SDLK_DOWN) {
        if (count == 0) return;
        int row = mapSelected < 0 ? 0 : mapSelected + (k == SDLK_UP ? -1 : 1);
        selectMapRow((row + count) % count);
    } else if (k == SDLK_RETURN || k == SDLK_KP_ENTER) {
        if (mapSelected >= 0 && mapSelected < count) startNewGame(mapResults[mapSelected]);
        else showMessage("No map matches \"" + mapSearch + "\"", pal::danger);
    } else if (k == SDLK_TAB) {
        if (mapSelected >= 0 && mapSelected < count) {
            int level = mapResults[mapSelected];
            if (level < ProfileManager::MAX_MAP_SAVES && savedWaves[level] > 0) loadGame(level);
            else showMessage("No saved game on this map", pal::danger);
        }
    } else if (k == SDLK_BACKSPACE) {
        if (!mapSearch.empty()) {
            mapSearch.pop_back();
            updateMapSearch();
        }
    } else if (k == SDLK_ESCAPE) {
        if (!mapSearch.empty()) {
            mapSearch.clear();
            updateMapSearch();
        } else {
            state = GameState::Menu;
        }
    } else if (k >= SDLK_1 && k <= SDLK_9) {
        int row = static_cast<int>(k - SDLK_1);
        if (row < count) startNewGame(mapResults[row]);
    }
}

void Game::handleMapSelectClick() {
    MapHit hit = screens.mapSelectHit(mouse.x, mouse.y, buildMapView());
    if (hit.action == ScreenAction::SelectMap) selectMapRow(hit.index);
    else if (hit.action == ScreenAction::PickMap && hit.index >= 0 && hit.index < static_cast<int>(mapResults.size()))
        startNewGame(mapResults[hit.index]);
    else if (hit.action == ScreenAction::ContinueMap && hit.index >= 0 && hit.index < static_cast<int>(mapResults.size()))
        loadGame(mapResults[hit.index]);
    else if (hit.action == ScreenAction::Back) state = GameState::Menu;
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
    if (troopBlocksCell(cellIndex(cx, cy))) return Placement::TroopInWay;
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
    case Placement::TroopInWay:
        showMessage("A Cannon Cart is in the way", pal::danger);
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
    shop.recordPurchase(type.typeId);
    updateSearch();
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
    if (Troop* tr = selectedTroop()) {
        int refund = tr->sellValue();
        gold += refund;
        effects.smoke(tr->getX(), tr->getY(), 8);
        effects.floatText(tr->getX(), tr->getY() - 30.0f, "+" + std::to_string(refund), pal::gold);
        tr->destroy();
        selectedTroopId = -1;
        return;
    }
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
    placingSpell = -1;
    placingTroop = false;
    selectedTroopId = -1;
    if (Tower* t = selectedTower()) t->setSelected(false);
    selectedTowerId = -1;
}

void Game::cancelPlacement() {
    placingType = -1;
    placingSpell = -1;
    placingTroop = false;
}

void Game::pickSpell(int kind, int tier) {
    if (!spells.validKind(kind) || !SpellBook::validTier(tier)) return;
    if (placingSpell == kind && placingTier == tier) {
        placingSpell = -1;
        return;
    }
    placingSpell = kind;
    placingTier = tier;
    placingType = -1;
    tab = TAB_SPELLS;
    placingTroop = false;
    if (Tower* t = selectedTower()) t->setSelected(false);
    selectedTowerId = -1;
}

void Game::cycleSpell(int kind) {
    if (!spells.validKind(kind)) return;
    if (placingSpell != kind) pickSpell(kind, 0);
    else if (placingTier + 1 < SPELL_TIERS) pickSpell(kind, placingTier + 1);
    else placingSpell = -1;
    tab = TAB_SPELLS;
}

bool Game::castSpell(int cx, int cy) {
    if (!spells.validKind(placingSpell) || !SpellBook::validTier(placingTier) || !insideMap(cx, cy)) return false;
    const SpellType& s = spells.type(placingSpell);
    const SpellTier& t = s.tiers[placingTier];
    if (!spells.ready(placingSpell)) {
        showMessage(s.title + " is recharging", pal::danger);
        return false;
    }
    if (gold < t.cost) {
        showMessage("Not enough gold", pal::danger);
        return false;
    }
    int centre = cellIndex(cx, cy);
    int cells[32];
    int n = engine_spell_area(centre, s.radius, cells, 32);
    if (n == 0) {
        showMessage("Spells can't land on rocks", pal::danger);
        return false;
    }
    float px = cellCentreX(centre);
    float py = cellCentreY(centre);
    if (s.kind == SpellKind::Freeze) {
        int frozen = 0;
        for (Enemy* e : enemies) {
            if (!e->isAlive()) continue;
            int ec = e->positionCell();
            for (int i = 0; i < n; i++) {
                if (cells[i] == ec) {
                    e->freeze(t.duration);
                    frozen++;
                    break;
                }
            }
        }
        for (int i = 0; i < n; i++) effects.flash(cellCentreX(cells[i]), cellCentreY(cells[i]), s.colour);
        effects.floatText(px, py - 34.0f, frozen > 0 ? std::to_string(frozen) + " frozen" : "Freeze", s.colour);
    } else {
        int kind = s.kind == SpellKind::Mud ? ZONE_MUD : ZONE_FIRE;
        int expires = static_cast<int>(std::lround((matchTime + t.duration) * 1000.0f));
        if (engine_zone_add(kind, centre, s.radius, expires) < 0) {
            showMessage("Too many spells on the field", pal::danger);
            return false;
        }
        if (s.kind == SpellKind::Fire) {
            for (int i = 0; i < n; i++) effects.sparks(cellCentreX(cells[i]), cellCentreY(cells[i]), s.colour);
        } else {
            effects.smoke(px, py, 8);
        }
    }
    effects.explosion(px, py, (s.radius + 0.5f) * CELL_SIZE, s.colour);
    gold -= t.cost;
    effects.floatText(px, py - 16.0f, "-" + std::to_string(t.cost), pal::gold);
    spells.startCooldown(placingSpell);
    shop.recordName(s.name);
    updateSearch();
    placingSpell = -1;
    return true;
}

void Game::setSearchFocus(bool focus) {
    searchFocused = focus;
    syncTextInput();
}

void Game::syncTextInput() {
    bool want = !showHelp && (state == GameState::Profiles || state == GameState::MapSelect ||
                              (state == GameState::Playing && searchFocused));
    if (want == textInputOn) return;
    if (want) SDL_StartTextInput();
    else SDL_StopTextInput();
    textInputOn = want;
}

void Game::updateSearch() {
    searchResults = shop.search(search);
    spellResults.clear();
    if (search.empty()) {
        spellResults = spells.all();
        return;
    }
    for (int id : shop.searchIds(search)) {
        int kind = id - SpellBook::SEARCH_ID_BASE;
        if (spells.validKind(kind)) spellResults.push_back(kind);
    }
    if (tab == TAB_SPELLS && spellResults.empty() && !searchResults.empty()) tab = TAB_TOWERS;
    else if (tab != TAB_SPELLS && searchResults.empty() && !spellResults.empty()) tab = TAB_SPELLS;
    else if (tab == TAB_TROOPS && !searchResults.empty()) tab = TAB_TOWERS;
}

void Game::handleSearchKey(SDL_Keycode k) {
    if (k == SDLK_BACKSPACE) {
        if (!search.empty()) {
            search.pop_back();
            updateSearch();
        }
    } else if (k == SDLK_RETURN || k == SDLK_KP_ENTER) {
        if (tab == TAB_SPELLS && !spellResults.empty()) {
            pickSpell(spellResults[0], 0);
            setSearchFocus(false);
            return;
        }
        if (searchResults.empty()) {
            showMessage("No tower or spell matches \"" + search + "\"", pal::danger);
            return;
        }
        placingType = searchResults[0];
        placingSpell = -1;
        if (Tower* t = selectedTower()) t->setSelected(false);
        selectedTowerId = -1;
        setSearchFocus(false);
    } else if (k == SDLK_ESCAPE) {
        search.clear();
        updateSearch();
        setSearchFocus(false);
    }
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

void Game::damageTower(Tower& t, int amount) {
    if (!t.isAlive()) return;
    effects.sparks(t.getX(), t.getY(), rgba(255, 170, 80));
    if (!t.takeDamage(amount)) return;
    engine_remove_tower(t.getCellX(), t.getCellY());
    mapVersion++;
    effects.explosion(t.getX(), t.getY(), 40.0f, rgba(255, 140, 60));
    effects.rubble(t.getX(), t.getY());
    if (selectedTowerId == t.getId()) selectedTowerId = -1;
    showMessage("Raiders destroyed your " + t.getType().title + "!", pal::danger);
}

void Game::damageDrone(Drone& d, int amount) {
    if (!d.isAlive()) return;
    if (!d.takeDamage(amount)) return;
    effects.explosion(d.getX(), d.getY() - Drone::ALTITUDE, 22.0f, rgba(120, 200, 255));
    showMessage(d.isOutbound() ? "A drone was shot down with its cargo" : "A drone was shot down", pal::danger);
}

void Game::deliverDrone(Drone& d, Tower& t) {
    if (d.getKind() == DroneKind::Ammo) {
        int added = t.resupply(t.ammoCrate());
        effects.floatText(t.getX(), t.getY() - 34.0f, "+" + std::to_string(added) + " ammo", pal::supply);
    } else {
        int healed = t.heal(t.repairAmount());
        effects.floatText(t.getX(), t.getY() - 34.0f, "+" + std::to_string(healed) + " hp", pal::good);
    }
    effects.flash(t.getX(), t.getY() - 10.0f, rgba(200, 240, 255));
}

void Game::sendDrone(DroneKind kind) {
    Tower* t = selectedTower();
    if (!t) {
        showMessage("Select a tower first", pal::uiText);
        return;
    }
    bool ammo = kind == DroneKind::Ammo;
    int cost = ammo ? AMMO_DRONE_COST : REPAIR_DRONE_COST;
    if (ammo && t->getAmmo().full()) {
        showMessage("That tower is fully stocked", pal::uiText);
        return;
    }
    if (!ammo && t->getHealth() >= t->getMaxHealth()) {
        showMessage("That tower is not damaged", pal::uiText);
        return;
    }
    for (const Drone* d : drones) {
        if (d->isAlive() && d->isOutbound() && d->getTowerId() == t->getId() && d->getKind() == kind) {
            showMessage("A drone is already on its way", pal::uiText);
            return;
        }
    }
    if (activeDrones() >= MAX_DRONES) {
        showMessage("All " + std::to_string(MAX_DRONES) + " drones are busy", pal::danger);
        return;
    }
    if (gold < cost) {
        showMessage("Not enough gold", pal::danger);
        return;
    }
    int home = engine_goal_cell();
    int target = cellIndex(t->getCellX(), t->getCellY());
    std::vector<int> path(MAP_W * MAP_H);
    if (engine_drone_path(home, target, path.data(), static_cast<int>(path.size())) == 0) {
        showMessage("No flight route to that tower", pal::danger);
        return;
    }
    gold -= cost;
    addEntity(std::make_unique<Drone>(kind, home, t->getId(), target));
    effects.floatText(cellCentreX(home), cellCentreY(home) - 40.0f, "-" + std::to_string(cost), pal::gold);
    showMessage(ammo ? "Ammo drone launched" : "Repair drone launched", pal::good);
}

void Game::updateDanger() {
    engine_clear_danger();
    const float reach = Enemy::RAIDER_RANGE + CELL_SIZE * 0.5f;
    for (const Enemy* e : enemies) {
        if (!e->isAlive() || !e->isRaider()) continue;
        int x0 = std::max(0, static_cast<int>((e->getX() - reach) / CELL_SIZE));
        int x1 = std::min(MAP_W - 1, static_cast<int>((e->getX() + reach) / CELL_SIZE));
        int y0 = std::max(0, static_cast<int>((e->getY() - reach) / CELL_SIZE));
        int y1 = std::min(MAP_H - 1, static_cast<int>((e->getY() + reach) / CELL_SIZE));
        for (int cy = y0; cy <= y1; cy++) {
            for (int cx = x0; cx <= x1; cx++) {
                float dx = cellCentreX(cellIndex(cx, cy)) - e->getX();
                float dy = cellCentreY(cellIndex(cx, cy)) - e->getY();
                if (dx * dx + dy * dy <= reach * reach) engine_add_danger(cellIndex(cx, cy), 1);
            }
        }
    }
}

void Game::spawnEnemy(EnemyKind kind, float healthScale) {
    int count = engine_spawn_count();
    if (count <= 0) return;
    int s = engine_spawn_at(std::rand() % count);
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
    return state == GameState::Profiles || state == GameState::Menu || state == GameState::MapSelect;
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
    syncTextInput();
}

void Game::closeProfiles() {
    nameInput.clear();
    cancelDelete();
    state = GameState::Menu;
    syncTextInput();
}

void Game::refreshProfileRows() {
    profileRows.clear();
    for (const Profile& p : profiles.all()) {
        int count = 0;
        std::string first;
        for (int level = 0; level < engine_level_count() && level < ProfileManager::MAX_MAP_SAVES; level++) {
            if (!SaveManager::exists(profiles.mapSavePath(p.id, level))) continue;
            if (count == 0) first = engine_level_name(level);
            count++;
        }
        std::string detail = "No saved game yet";
        if (count == 1) detail = "Saved: " + first;
        else if (count > 1) detail = "Saved on " + std::to_string(count) + " maps";
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
    records.load(profiles.recordsPath(id));
    std::string legacy = profiles.savePath(id);
    SaveData old;
    std::string error;
    if (SaveManager::load(legacy, old, error) && old.level >= 0 && old.level < ProfileManager::MAX_MAP_SAVES &&
        !SaveManager::exists(profiles.mapSavePath(id, old.level)))
        std::rename(legacy.c_str(), profiles.mapSavePath(id, old.level).c_str());
    refreshSavedMaps();
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
        refreshSavedMaps();
        records.clear();
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
    if (state == GameState::MapSelect && !showHelp) {
        bool changed = false;
        for (const char* c = text; *c; c++) {
            unsigned char u = static_cast<unsigned char>(*c);
            if (!(std::isalpha(u) || *c == ' ') || static_cast<int>(mapSearch.size()) >= ShopManager::MAX_SEARCH_LENGTH) continue;
            if (*c == ' ' && (mapSearch.empty() || mapSearch.back() == ' ')) continue;
            mapSearch.push_back(static_cast<char>(std::tolower(u)));
            changed = true;
        }
        if (changed) updateMapSearch();
        return;
    }
    if (state == GameState::Playing && searchFocused && !showHelp) {
        bool changed = false;
        for (const char* c = text; *c; c++) {
            if (!ShopManager::allowedSearchChar(*c) || static_cast<int>(search.size()) >= ShopManager::MAX_SEARCH_LENGTH) continue;
            if (*c == ' ' && (search.empty() || search.back() == ' ')) continue;
            search.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(*c))));
            changed = true;
        }
        if (changed) updateSearch();
        return;
    }
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
    data.level = levelIndex;
    data.timeMs = timeKnown ? static_cast<int>(std::lround(playTime * 1000.0f)) : -1;
    for (const Troop* tr : troops) {
        if (tr->isAlive()) data.troops.push_back({cellX(tr->getPost()), cellY(tr->getPost()), tr->getHealth()});
    }
    for (const Tower* t : towers) {
        if (!t->isAlive()) continue;
        data.towers.push_back({t->getObjectId(), t->getTypeId(), t->getCellX(), t->getCellY(), t->getLevel(),
                               t->getAmmo().current(), t->getHealth()});
    }
    std::size_t raw = 0;
    std::size_t stored = 0;
    std::string error;
    if (levelIndex < 0 || levelIndex >= ProfileManager::MAX_MAP_SAVES) {
        showMessage("This map cannot be saved", pal::danger);
        return;
    }
    if (SaveManager::save(data, profiles.mapSavePath(activeProfile, levelIndex), raw, stored, error)) {
        refreshSavedMaps();
        showMessage(std::string("Saved ") + engine_level_name(levelIndex) + ": " + std::to_string(raw) + " -> " +
                        std::to_string(stored) + " bytes",
                    pal::good);
    } else {
        showMessage(error, pal::danger);
    }
}

void Game::loadGame(int level) {
    if (activeProfile < 0) {
        showMessage("Choose a profile first", pal::danger);
        return;
    }
    if (level < 0 || level >= ProfileManager::MAX_MAP_SAVES) return;
    SaveData data;
    std::string error;
    if (!SaveManager::load(profiles.mapSavePath(activeProfile, level), data, error)) {
        showMessage(std::string("No saved game on ") + engine_level_name(level), pal::danger);
        return;
    }
    levelIndex = (level < engine_level_count()) ? level : 0;
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
        if (st.ammo >= 0) t->setAmmo(st.ammo);
        if (st.health > 0) t->restoreHealth(st.health);
        addEntity(std::move(t));
        maxId = std::max(maxId, st.objectId);
    }
    for (const SavedTroop& st : data.troops) {
        if (activeTroops() >= Troop::MAX_TROOPS || !insideMap(st.cellX, st.cellY)) continue;
        int type = engine_cell_type(st.cellX, st.cellY);
        if ((type != CELL_BUILDABLE && type != CELL_PATH) || towerAt(st.cellX, st.cellY)) continue;
        std::unique_ptr<Troop> tr = std::make_unique<Troop>(cellIndex(st.cellX, st.cellY));
        if (st.health > 0) tr->restoreHealth(st.health);
        addEntity(std::move(tr));
    }
    mergePending();
    nextTowerObjectId = maxId + 1;
    towersBuilt = static_cast<int>(towers.size());
    waves.restartAt(data.wave);
    wavesCleared = waves.waveNumber() - 1;
    timeKnown = data.timeMs >= 0;
    playTime = timeKnown ? data.timeMs / 1000.0f : 0.0f;
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
    SDL_FPoint logical = gfx.toLogical(mx, my);
    mouse.x = logical.x;
    mouse.y = logical.y;

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
        if (e.key.keysym.sym == SDLK_F11 ||
            (e.key.keysym.sym == SDLK_RETURN && (e.key.keysym.mod & KMOD_ALT) != 0)) {
            if (e.key.repeat == 0) gfx.toggleFullscreen();
            break;
        }
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
    syncTextInput();
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
        if (k == SDLK_RETURN || k == SDLK_SPACE) openMapSelect();
        else if (k == SDLK_ESCAPE) running = false;
        return;
    }
    if (state == GameState::MapSelect) {
        handleMapSelectKey(k);
        return;
    }
    if (state == GameState::Paused) {
        if (k == SDLK_ESCAPE || k == SDLK_p) state = GameState::Playing;
        return;
    }
    if (state == GameState::GameOver || state == GameState::Victory) {
        if (k == SDLK_RETURN) startNewGame(levelIndex);
        return;
    }

    if (searchFocused) {
        handleSearchKey(k);
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
        if (placingType >= 0 || placingSpell >= 0 || placingTroop) cancelPlacement();
        else if (selectedTowerId >= 0 || selectedTroopId >= 0) {
            selectedTowerId = -1;
            selectedTroopId = -1;
        }
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
    case SDLK_a:
        sendDrone(DroneKind::Ammo);
        break;
    case SDLK_r:
        sendDrone(DroneKind::Repair);
        break;
    case SDLK_SLASH:
        setSearchFocus(true);
        break;
    case SDLK_TAB:
        tab = (tab + 1) % 3;
        break;
    case SDLK_z:
        cycleSpell(static_cast<int>(SpellKind::Freeze));
        break;
    case SDLK_x:
        cycleSpell(static_cast<int>(SpellKind::Mud));
        break;
    case SDLK_c:
        cycleSpell(static_cast<int>(SpellKind::Fire));
        break;
    case SDLK_t:
        pickTroop();
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
            openMapSelect();
            break;
        case ScreenAction::Continue:
            continueFromMenu();
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
    if (state == GameState::MapSelect) {
        if (button != SDL_BUTTON_LEFT) return;
        handleMapSelectClick();
        return;
    }
    if (state == GameState::Paused) {
        if (button != SDL_BUTTON_LEFT) return;
        switch (screens.pauseHit(mouse.x, mouse.y)) {
        case ScreenAction::Resume:
            state = GameState::Playing;
            break;
        case ScreenAction::Restart:
            startNewGame(levelIndex);
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
        if (a == ScreenAction::Restart) startNewGame(levelIndex);
        else if (a == ScreenAction::MainMenu) {
            resetMatch();
            state = GameState::Menu;
        }
        return;
    }

    if (button == SDL_BUTTON_RIGHT) {
        setSearchFocus(false);
        if (placingType >= 0 || placingSpell >= 0 || placingTroop) cancelPlacement();
        else {
            selectedTowerId = -1;
            selectedTroopId = -1;
        }
        return;
    }

    if (mouse.x < PANEL_X) {
        setSearchFocus(false);
        int cx = 0;
        int cy = 0;
        if (mouseCell(cx, cy)) handleMapClick(cx, cy);
        else if (placingType < 0 && placingSpell < 0 && !placingTroop) {
            selectedTowerId = -1;
            selectedTroopId = -1;
        }
    } else {
        handlePanelClick();
    }
}

void Game::handleMapClick(int cx, int cy) {
    if (placingSpell >= 0) {
        castSpell(cx, cy);
        return;
    }
    if (placingTroop) {
        tryPlaceTroop(cx, cy);
        return;
    }
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
    Troop* tr = t ? nullptr : troopAt(cellIndex(cx, cy));
    selectedTroopId = tr ? tr->getId() : -1;
}

void Game::handlePanelClick() {
    PanelModel model = buildPanelModel();
    PanelHit hit = panel.hitTest(mouse.x, mouse.y, model);
    if (hit.action != PanelAction::FocusSearch) setSearchFocus(false);
    switch (hit.action) {
    case PanelAction::StartWave:
        requestWaveStart(true);
        break;
    case PanelAction::PickType:
        pickType(hit.typeId);
        break;
    case PanelAction::FocusSearch:
        setSearchFocus(true);
        break;
    case PanelAction::TabTowers:
        tab = TAB_TOWERS;
        break;
    case PanelAction::TabSpells:
        tab = TAB_SPELLS;
        break;
    case PanelAction::TabTroops:
        tab = TAB_TROOPS;
        break;
    case PanelAction::PickTroop:
        pickTroop();
        break;
    case PanelAction::PickSpell:
        pickSpell(hit.typeId / SPELL_TIERS, hit.typeId % SPELL_TIERS);
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
        loadGame(levelIndex);
        break;
    case PanelAction::Speed:
        fastSpeed = !fastSpeed;
        break;
    case PanelAction::Pause:
        state = GameState::Paused;
        break;
    case PanelAction::AmmoDrone:
        sendDrone(DroneKind::Ammo);
        break;
    case PanelAction::RepairDrone:
        sendDrone(DroneKind::Repair);
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
    syncTextInput();

    if (state != GameState::Playing || showHelp) return;

    const float dt = realDt * (fastSpeed ? 2.0f : 1.0f);
    matchTime += dt;
    playTime += realDt;
    engine_zone_update(static_cast<int>(std::lround(matchTime * 1000.0f)));
    spells.update(dt);
    castleFlash = std::max(0.0f, castleFlash - dt);
    shake = std::max(0.0f, shake - dt * 30.0f);

    waves.update(dt, *this, aliveEnemyCount());
    updateDanger();
    for (std::unique_ptr<Entity>& e : entities) {
        if (e->isAlive()) e->update(dt, *this);
    }
    mergePending();
    removeDead();
    effects.update(dt);

    for (Tower* t : towers) t->setSelected(t->getId() == selectedTowerId);
    if (selectedTowerId >= 0 && !selectedTower()) selectedTowerId = -1;
    for (Troop* t : troops) t->setSelected(t->getId() == selectedTroopId);
    if (selectedTroopId >= 0 && !selectedTroop()) selectedTroopId = -1;

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
        placingSpell = -1;
        placingTroop = false;
        setSearchFocus(false);
        finishMatch(false);
    } else if (waves.allWavesDone() && aliveEnemyCount() == 0) {
        state = GameState::Victory;
        placingType = -1;
        placingSpell = -1;
        placingTroop = false;
        setSearchFocus(false);
        finishMatch(true);
    }
}

void Game::finishMatch(bool won) {
    if (recordDone) return;
    recordDone = true;
    lastScore = RecordBook::scoreFor(wavesCleared, enemiesDefeated, castleHealth, won);
    std::string key = RecordBook::mapKey(engine_level_name(levelIndex));
    int timeMs = timeKnown ? static_cast<int>(std::lround(playTime * 1000.0f)) : -1;
    if (activeProfile >= 0) {
        lastResult = records.submit(key, lastScore, won, timeMs);
        lastBest = records.get(key);
    } else {
        lastResult = {false, false};
        lastBest = {lastScore, won ? timeMs : -1, 1, won ? 1 : 0};
    }
}

EndStats Game::buildEndStats() const {
    EndStats s;
    s.victory = state == GameState::Victory;
    s.wavesCleared = wavesCleared;
    s.totalWaves = waves.totalWaves();
    s.towersBuilt = towersBuilt;
    s.enemiesDefeated = enemiesDefeated;
    s.castleHealth = castleHealth;
    s.mapName = engine_level_name(levelIndex);
    s.score = lastScore;
    s.bestScore = lastBest.bestScore;
    s.newBestScore = lastResult.newBestScore;
    s.timeMs = static_cast<int>(std::lround(playTime * 1000.0f));
    s.timeKnown = timeKnown;
    s.bestTimeMs = lastBest.bestTimeMs;
    s.newBestTime = lastResult.newBestTime;
    return s;
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
    m.results = searchResults;
    m.placingType = placingType;
    m.search = search;
    m.searchFocused = searchFocused;
    m.shop = &shop;
    m.selected = selectedTower();
    m.fastSpeed = fastSpeed;
    m.dronesActive = activeDrones();
    m.maxDrones = MAX_DRONES;
    m.ammoDroneCost = AMMO_DRONE_COST;
    m.repairDroneCost = REPAIR_DRONE_COST;
    m.tab = tab;
    m.troop = selectedTroop();
    m.troopCount = activeTroops();
    m.maxTroops = Troop::MAX_TROOPS;
    m.troopCost = Troop::COST;
    m.placingTroop = placingTroop;
    m.spellResults = spellResults;
    m.spells = &spells;
    m.placingSpell = placingSpell;
    m.placingTier = placingTier;
    return m;
}

void Game::drawDanger() {
    if (activeDrones() == 0) return;
    gfx.useGround(0.0f);
    for (int cy = 0; cy < MAP_H; cy++) {
        for (int cx = 0; cx < MAP_W; cx++) {
            int d = engine_danger(cellIndex(cx, cy));
            if (d <= 0) continue;
            gfx.fillRect(cx * CELL_SIZE + 1.0f, cy * CELL_SIZE + 1.0f, CELL_SIZE - 2.0f, CELL_SIZE - 2.0f,
                         rgba(230, 70, 40, std::min(90, 26 * d)));
        }
    }
    gfx.useScreen();
}

void Game::drawZones(bool flames) {
    if (engine_zone_count() == 0) return;
    const int now = static_cast<int>(std::lround(matchTime * 1000.0f));
    const float t = gfx.time();
    for (int cy = 0; cy < MAP_H; cy++) {
        for (int cx = 0; cx < MAP_W; cx++) {
            int c = cellIndex(cx, cy);
            float x = cx * CELL_SIZE;
            float y = cy * CELL_SIZE;
            float mx = x + CELL_SIZE * 0.5f;
            float my = y + CELL_SIZE * 0.5f;
            if (!flames && engine_zone_at(c, ZONE_MUD) > 0) {
                float fade = std::clamp((engine_zone_until(c, ZONE_MUD) - now) / 1000.0f, 0.0f, 1.0f);
                int a = static_cast<int>(210 * fade);
                gfx.useGround(0.0f);
                gfx.fillRoundRect(x + 2.0f, y + 2.0f, CELL_SIZE - 4.0f, CELL_SIZE - 4.0f, 16.0f, rgba(86, 60, 34, a));
                for (int k = 0; k < 3; k++) {
                    float ox = std::sin(c * 1.7f + k * 2.3f) * 12.0f;
                    float oy = std::cos(c * 2.1f + k * 1.9f) * 10.0f;
                    gfx.fillEllipse(mx + ox, my + oy, 8.0f, 6.0f, rgba(122, 88, 52, a));
                }
                float bob = 0.5f + 0.5f * std::sin(t * 2.5f + c);
                gfx.fillCircle(mx + std::sin(c * 3.1f) * 9.0f, my - 4.0f, 1.5f + bob * 1.5f, rgba(170, 130, 86, a));
            }
            if (engine_zone_at(c, ZONE_FIRE) > 0) {
                float fade = std::clamp((engine_zone_until(c, ZONE_FIRE) - now) / 1000.0f, 0.0f, 1.0f);
                int a = static_cast<int>(255 * fade);
                if (!flames) {
                    gfx.useGround(0.0f);
                    gfx.fillRoundRect(x + 3.0f, y + 3.0f, CELL_SIZE - 6.0f, CELL_SIZE - 6.0f, 12.0f,
                                      rgba(60, 20, 10, a * 140 / 255));
                    gfx.glow(mx, my, 32.0f, 32.0f, rgba(255, 110, 30, a * 90 / 255));
                    continue;
                }
                for (int k = 0; k < 4; k++) {
                    float fx = mx + ((k % 2) ? 9.0f : -9.0f);
                    float fy = my + ((k < 2) ? -9.0f : 9.0f);
                    float h = 12.0f + 8.0f * (0.5f + 0.5f * std::sin(t * 9.0f + c + k * 1.7f));
                    gfx.useBillboard(fx, fy, 0.0f);
                    gfx.fillTriangle({fx - 5.0f, fy}, {fx + 5.0f, fy}, {fx, fy - h}, rgba(240, 96, 30, a * 220 / 255));
                    gfx.fillTriangle({fx - 2.5f, fy}, {fx + 2.5f, fy}, {fx, fy - h * 0.55f}, rgba(255, 210, 90, a));
                }
            }
        }
    }
    gfx.useScreen();
}

void Game::drawSpellPreview(int cx, int cy, bool ground) {
    const SpellType& s = spells.type(placingSpell);
    const SpellTier& t = s.tiers[placingTier];
    int centre = cellIndex(cx, cy);
    int cells[32];
    int n = engine_spell_area(centre, s.radius, cells, 32);
    std::string why;
    if (n == 0) why = "Can't cast on rocks";
    else if (!spells.ready(placingSpell)) why = "Ready in " + std::to_string(static_cast<int>(std::ceil(spells.cooldownLeft(placingSpell)))) + "s";
    else if (gold < t.cost) why = "Not enough gold";
    bool ok = why.empty();
    Color c = ok ? s.colour : rgba(236, 86, 72);
    float x = cx * CELL_SIZE;
    float y = cy * CELL_SIZE;
    if (ground) {
        gfx.useGround(0.0f);
        if (n == 0) {
            gfx.fillRoundRect(x + 2.0f, y + 2.0f, CELL_SIZE - 4.0f, CELL_SIZE - 4.0f, 7.0f, withAlpha(c, 70));
            gfx.strokeRoundRect(x + 2.0f, y + 2.0f, CELL_SIZE - 4.0f, CELL_SIZE - 4.0f, 7.0f, 2.4f, withAlpha(c, 220));
        }
        for (int i = 0; i < n; i++) {
            float ax = cellX(cells[i]) * CELL_SIZE;
            float ay = cellY(cells[i]) * CELL_SIZE;
            gfx.fillRoundRect(ax + 2.0f, ay + 2.0f, CELL_SIZE - 4.0f, CELL_SIZE - 4.0f, 8.0f, withAlpha(c, i == 0 ? 90 : 56));
            gfx.strokeRoundRect(ax + 2.0f, ay + 2.0f, CELL_SIZE - 4.0f, CELL_SIZE - 4.0f, 8.0f, 2.0f, withAlpha(c, 180));
        }
        gfx.useScreen();
        return;
    }
    float mx = x + CELL_SIZE * 0.5f;
    float my = y + CELL_SIZE * 0.5f;
    float prev = gfx.alpha();
    gfx.setAlpha(ok ? 0.95f : 0.55f);
    gfx.useBillboard(mx, my, 22.0f);
    SpellBook::drawIcon(gfx, placingSpell, mx, my, 14.0f, gfx.time());
    gfx.setAlpha(prev);
    gfx.useScreen();
    if (!ok) drawTip(why, mx, my);
}

void Game::drawTip(const std::string& why, float wx, float wy) {
    SDL_FPoint p = gfx.project(wx, wy, 58.0f);
    float w = gfx.textWidth(FontId::SmallBold, why) + 16.0f;
    float tx = std::clamp(p.x, w * 0.5f + 4.0f, PANEL_X - w * 0.5f - 4.0f);
    float ty = std::max(4.0f, p.y - 9.0f);
    gfx.fillRoundRect(tx - w * 0.5f, ty, w, 18.0f, 9.0f, rgba(20, 10, 10, 210));
    gfx.textCentered(FontId::SmallBold, why, tx, ty + 9.0f, rgba(255, 170, 160));
}

bool Game::mouseCell(int& cx, int& cy) const {
    if (mouse.x < 0.0f || mouse.x >= PANEL_X || mouse.y < 0.0f || mouse.y >= WINDOW_H) return false;
    SDL_FPoint w = MapView::battlefieldView().toWorld(mouse.x, mouse.y);
    if (w.x < 0.0f || w.y < 0.0f) return false;
    cx = static_cast<int>(w.x / CELL_SIZE);
    cy = static_cast<int>(w.y / CELL_SIZE);
    return insideMap(cx, cy);
}

void Game::drawEntities(int layer) const {
    for (const std::unique_ptr<Entity>& e : entities) {
        if (e->isAlive() && e->layer() == layer) e->draw(gfx);
    }
}

void Game::drawSelectedRange() {
    Tower* t = selectedTower();
    if (!t) return;
    gfx.useGround(0.0f);
    gfx.fillCircle(t->getX(), t->getY(), t->range(), rgba(234, 186, 78, 26));
    gfx.strokeCircle(t->getX(), t->getY(), t->range(), 2.0f, rgba(234, 186, 78, 170));
    gfx.useScreen();
}

void Game::drawPlacementGround() {
    int cx = 0;
    int cy = 0;
    if (!mouseCell(cx, cy)) return;
    float x = cx * CELL_SIZE;
    float y = cy * CELL_SIZE;
    float mxc = x + CELL_SIZE * 0.5f;
    float myc = y + CELL_SIZE * 0.5f;
    if (placingSpell >= 0) {
        drawSpellPreview(cx, cy, true);
        return;
    }
    gfx.useGround(0.0f);
    if (placingTroop) {
        bool ok = troopProblem(cx, cy).empty();
        Color c = ok ? pal::supply : rgba(236, 86, 72);
        gfx.fillRoundRect(x + 2.0f, y + 2.0f, CELL_SIZE - 4.0f, CELL_SIZE - 4.0f, 7.0f, withAlpha(c, 70));
        gfx.strokeRoundRect(x + 2.0f, y + 2.0f, CELL_SIZE - 4.0f, CELL_SIZE - 4.0f, 7.0f, 2.4f, withAlpha(c, 220));
        gfx.fillCircle(mxc, myc, Troop::GUARD_RADIUS, withAlpha(c, 16));
        gfx.strokeCircle(mxc, myc, Troop::GUARD_RADIUS, 1.6f, withAlpha(c, 120));
        gfx.useScreen();
        return;
    }
    if (placingType < 0) {
        if (towerAt(cx, cy) && !selectedTower()) gfx.strokeCircle(mxc, myc, 25.0f, 2.0f, rgba(255, 255, 255, 110));
        gfx.useScreen();
        return;
    }
    Placement p = checkPlacement(cx, cy, placingType);
    bool ok = p == Placement::Ok;
    Color c = ok ? rgba(110, 220, 120) : rgba(236, 86, 72);
    gfx.fillRoundRect(x + 2.0f, y + 2.0f, CELL_SIZE - 4.0f, CELL_SIZE - 4.0f, 7.0f, withAlpha(c, 70));
    gfx.strokeRoundRect(x + 2.0f, y + 2.0f, CELL_SIZE - 4.0f, CELL_SIZE - 4.0f, 7.0f, 2.4f, withAlpha(c, 220));
    if (p != Placement::Occupied && p != Placement::OffMap) {
        float range = shop.type(placingType).range;
        gfx.fillCircle(mxc, myc, range, withAlpha(c, 20));
        gfx.strokeCircle(mxc, myc, range, 1.8f, withAlpha(c, 140));
    }
    gfx.useScreen();
}

void Game::drawPlacementGhost() {
    int cx = 0;
    int cy = 0;
    if (!mouseCell(cx, cy)) return;
    float mxc = cx * CELL_SIZE + CELL_SIZE * 0.5f;
    float myc = cy * CELL_SIZE + CELL_SIZE * 0.5f;
    if (placingSpell >= 0) {
        drawSpellPreview(cx, cy, false);
        return;
    }
    if (placingTroop) {
        std::string why = troopProblem(cx, cy);
        float prev = gfx.alpha();
        gfx.setAlpha(why.empty() ? 0.85f : 0.45f);
        Troop::drawModel(gfx, mxc, myc, 1.25f, PI_F * 0.25f, PI_F * 0.25f, 0.0f, 0.0f);
        gfx.setAlpha(prev);
        if (!why.empty()) drawTip(why, mxc, myc);
        return;
    }
    if (placingType < 0) return;
    Placement p = checkPlacement(cx, cy, placingType);
    if (p == Placement::Occupied || p == Placement::OffMap) return;
    bool ok = p == Placement::Ok;
    float prev = gfx.alpha();
    gfx.setAlpha(ok ? 0.8f : 0.45f);
    gfx.useGround(0.0f);
    Tower::drawModel(gfx, placingType, mxc, myc, 1.0f, -PI_F * 0.25f, 1, 0.0f, true);
    gfx.useScreen();
    gfx.setAlpha(prev);
    if (!ok && p != Placement::NotBuildable) {
        const char* why = "";
        if (p == Placement::BlocksPath) why = "Would block the path";
        else if (p == Placement::EnemyInWay) why = "Enemy in the way";
        else if (p == Placement::NoGold) why = "Not enough gold";
        else if (p == Placement::TroopInWay) why = "A cart is in the way";
        drawTip(why, mxc, myc);
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

void Game::drawSortedObjects() {
    struct Item {
        float depth;
        int order;
        int cell;
        const Entity* entity;
    };
    std::vector<Item> items;
    const IsoView view = MapView::battlefieldView();
    for (int c = 0; c < MAP_W * MAP_H; c++) {
        if (map.hasCellObject(c)) items.push_back({view.depth(cellCentreX(c), cellCentreY(c)), 0, c, nullptr});
    }
    int goal = map.goalCell();
    if (goal >= 0) items.push_back({view.depth(cellCentreX(goal), cellCentreY(goal)) + 0.45f, 1, goal, nullptr});
    for (const std::unique_ptr<Entity>& e : entities) {
        if (!e->isAlive() || e->layer() > 1) continue;
        items.push_back({view.depth(e->getX(), e->getY()), 2 + e->layer(), -1, e.get()});
    }
    std::stable_sort(items.begin(), items.end(), [](const Item& a, const Item& b) {
        if (a.depth != b.depth) return a.depth < b.depth;
        return a.order < b.order;
    });
    for (const Item& it : items) {
        if (it.entity) it.entity->draw(gfx);
        else if (it.order == 1) map.drawCastle(gfx, castleFlash);
        else map.drawCellObject(gfx, it.cell);
        gfx.useScreen();
    }
}

void Game::drawBattlefield() {
    float sx = 0.0f;
    float sy = 0.0f;
    if (shake > 0.0f) {
        sx = (static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX) - 0.5f) * shake;
        sy = (static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX) - 0.5f) * shake;
    }
    gfx.setCamera(MapView::battlefieldView());
    gfx.setClip(0, 0, PANEL_X, WINDOW_H);
    map.drawBackdrop(gfx);
    gfx.setOffset(sx, sy);
    map.drawGround(gfx);
    map.drawLava(gfx);
    const bool playing = state == GameState::Playing && !showHelp;
    if (playing && (placingType >= 0 || placingSpell >= 0 || placingTroop)) map.drawGrid(gfx, 1.0f);
    if (!inMenus()) {
        drawDanger();
        drawZones(false);
        drawSelectedRange();
    }
    map.drawPortals(gfx);
    if (playing) drawPlacementGround();
    if (!inMenus()) drawZones(true);
    drawSortedObjects();
    drawEntities(2);
    drawEntities(3);
    effects.draw(gfx);
    if (playing) drawPlacementGhost();
    gfx.useScreen();
    map.drawAmbient(gfx);
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
    } else if (state == GameState::MapSelect) {
        screens.drawMapSelect(gfx, mouse, buildMapView());
    } else {
        panel.draw(gfx, buildPanelModel(), mouse);
        if (state == GameState::Paused) screens.drawPause(gfx, mouse);
        else if (state == GameState::GameOver || state == GameState::Victory) {
            screens.drawEnd(gfx, mouse, buildEndStats());
        }
    }
    if (showHelp) screens.drawHelp(gfx, mouse);
    if (inMenus() && messageTimer > 0.0f) drawMessage();

    gfx.endFrame();
}
