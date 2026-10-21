#ifndef SIDE_PANEL_H
#define SIDE_PANEL_H

#include "shop_manager.h"
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
    const ShopManager* shop;
    const Tower* selected;
    bool fastSpeed;
};

enum class PanelAction { None, StartWave, PickType, Upgrade, Sell, Save, Load, Speed, Pause };

struct PanelHit {
    PanelAction action;
    int typeId;
};

class SidePanel {
public:
    SidePanel();

    PanelHit hitTest(float mx, float my, const PanelModel& model) const;
    int cardAt(float mx, float my, const PanelModel& model) const;
    void draw(Gfx& g, const PanelModel& model, const MouseState& mouse) const;

private:
    float x0;
    float width;
    Button waveButton;
    Button upgradeButton;
    Button sellButton;
    Button saveButton;
    Button loadButton;
    Button speedButton;
    Button pauseButton;
    float cardsTop;
    float cardHeight;
    float cardGap;
    float infoTop;

    SDL_FRect cardRect(int slot) const;
    void drawHeader(Gfx& g, const PanelModel& m) const;
    void drawWaveButton(Gfx& g, const PanelModel& m, const MouseState& mouse) const;
    void drawCards(Gfx& g, const PanelModel& m, const MouseState& mouse) const;
    void drawInfo(Gfx& g, const PanelModel& m, const MouseState& mouse) const;
    void drawStatRow(Gfx& g, float y, const std::string& label, const std::string& value) const;
};

#endif
