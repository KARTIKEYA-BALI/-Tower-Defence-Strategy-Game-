#ifndef TROOP_H
#define TROOP_H

#include "entity.h"
#include "gfx.h"
#include <vector>

class Enemy;

enum class TroopMode { Guard, Hunt, Fight, Return };

class Troop : public Entity {
public:
    static constexpr int COST = 90;
    static constexpr int HEALTH = 260;
    static constexpr int DAMAGE = 24;
    static constexpr float SPLASH = 22.0f;
    static constexpr float RANGE = 135.0f;
    static constexpr float RELOAD = 1.2f;
    static constexpr float SPEED = 64.0f;
    static constexpr float SHOT_SPEED = 380.0f;
    static constexpr float GUARD_RADIUS = 175.0f;
    static constexpr float REGEN = 4.0f;
    static constexpr float REPLAN = 0.5f;
    static constexpr int MAX_TROOPS = 2;

    explicit Troop(int postCell);

    void update(float dt, Game& game) override;
    void draw(Gfx& g) const override;
    int layer() const override { return 1; }

    int getPost() const { return post; }
    int getCell() const { return cell; }
    int nextCell() const { return path.size() > 1 ? path[1] : cell; }
    int positionCell() const;
    TroopMode getMode() const { return mode; }
    const char* modeName() const;
    int sellValue() const { return COST / 2; }
    void setSelected(bool s) { selected = s; }

    static void drawModel(Gfx& g, float wx, float wy, float s, float heading, float aim, float wheel, float recoil);
    static void drawIcon(Gfx& g, float cx, float cy, float scale);

protected:
    void onDamaged(int amount) override;

private:
    int post;
    int cell;
    int targetId;
    TroopMode mode;
    std::vector<int> path;
    int planGoal;
    float replanTimer;
    float heading;
    float aim;
    float cooldown;
    float recoil;
    float hitFlash;
    float wheel;
    float regenCarry;
    bool selected;

    Enemy* chooseTarget(Game& game);
    void moveToward(int goalCell, float dt);
    void fire(Game& game, const Enemy& target);
};

#endif
