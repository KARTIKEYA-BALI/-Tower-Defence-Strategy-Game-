#ifndef SIDE_PANEL_H
#define SIDE_PANEL_H

#include "shop_manager.h"
#include "spell_book.h"
#include "troop.h"
#include "ui.h"
#include <string>
#include <vector>

class Tower;

struct PanelModel {
    int health;
    int maxHealth;
    int gold;
    int wave;
    int totalWaves;
    bool waveActive;
    bool allWavesDone;
    float countdown;
    int earlyBonus;
    int enemiesLeft;
    std::vector<int> results;
    int placingType;
    std::string search;
    bool searchFocused;
    const ShopManager* shop;
    const Tower* selected;
    bool fastSpeed;
    int dronesActive;
    int maxDrones;
    int ammoDroneCost;
    int repairDroneCost;
    int tab;
    const Troop* troop;
    int troopCount;
    int maxTroops;
    int troopCost;
    bool placingTroop;
    std::vector<int> spellResults;
    const SpellBook* spells;
    int placingSpell;
    int placingTier;
};

enum class PanelAction {
    None,
    StartWave,
    PickType,
    FocusSearch,
    Upgrade,
    Sell,
    Save,
    Load,
    Speed,
    Pause,
    AmmoDrone,
    RepairDrone,
    TabTowers,
    TabSpells,
    TabTroops,
    PickSpell,
    PickTroop
};

struct PanelHit {
    PanelAction action;
    int typeId;
};

class SidePanel {
public:
    SidePanel();

    PanelHit hitTest(float mx, float my, const PanelModel& model) const;
    int cardAt(float mx, float my, const PanelModel& model) const;
    int spellCardAt(float mx, float my, const PanelModel& model) const;
    int spellChipAt(float mx, float my, const PanelModel& model) const;
    void draw(Gfx& g, const PanelModel& model, const MouseState& mouse) const;

private:
    float x0;
    float width;
    Button waveButton;
    TextBox searchBox;
    Button upgradeButton;
    Button sellButton;
    Button ammoButton;
    Button repairButton;
    Button saveButton;
    Button loadButton;
    Button speedButton;
    Button pauseButton;
    float cardsTop;
    float cardHeight;
    float cardGap;
    float infoTop;

    float spellHeight;
    float spellGap;

    SDL_FRect cardRect(int slot) const;
    SDL_FRect tabRect(int tab) const;
    SDL_FRect spellRect(int slot) const;
    SDL_FRect chipRect(int slot, int tier) const;
    void drawTabs(Gfx& g, const PanelModel& m, const MouseState& mouse) const;
    void drawSpells(Gfx& g, const PanelModel& m, const MouseState& mouse) const;
    void drawSpellInfo(Gfx& g, const PanelModel& m, const MouseState& mouse) const;
    void drawTroops(Gfx& g, const PanelModel& m, const MouseState& mouse) const;
    void drawTroopInfo(Gfx& g, const PanelModel& m, const MouseState& mouse) const;
    bool troopCardHit(float mx, float my, const PanelModel& m) const;
    void drawHeader(Gfx& g, const PanelModel& m) const;
    void drawWaveButton(Gfx& g, const PanelModel& m, const MouseState& mouse) const;
    void drawCards(Gfx& g, const PanelModel& m, const MouseState& mouse) const;
    void drawInfo(Gfx& g, const PanelModel& m, const MouseState& mouse) const;
    void drawStatRow(Gfx& g, float y, const std::string& label, const std::string& value) const;
};

#endif
