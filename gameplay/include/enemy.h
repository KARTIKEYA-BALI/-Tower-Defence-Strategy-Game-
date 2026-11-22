#ifndef ENEMY_H
#define ENEMY_H

#include "entity.h"
#include "gfx.h"
#include <cstdint>

enum class EnemyKind { Grunt, Runner, Brute, Warlord, Raider };

struct EnemyStats {
    const char* name;
    int health;
    float speed;
    int reward;
    int damageToCastle;
    float radius;
    Color body;
    Color trim;
};

const EnemyStats& enemyStats(EnemyKind kind);

class Enemy : public Entity {
public:
    static constexpr float RAIDER_RANGE = 95.0f;
    static constexpr int RAIDER_DAMAGE = 9;
    static constexpr float RAIDER_RELOAD = 1.3f;

    Enemy(EnemyKind kind, int spawnCell, float healthScale);

    void update(float dt, Game& game) override;
    void draw(Gfx& g) const override;
    int layer() const override { return 1; }

    EnemyKind getKind() const { return kind; }
    int getCell() const { return cell; }
    int getTargetCell() const { return targetCell; }
    float getRadius() const { return radius; }
    int getReward() const { return reward; }
    float getVelX() const { return velX; }
    float getVelY() const { return velY; }
    bool isRaider() const { return kind == EnemyKind::Raider; }
    bool isFrozen() const { return freezeTimer > 0.0f; }
    bool isAttackingTower() const { return attackingTower; }
    bool isMuddy() const { return inMud; }
    bool isBurning() const { return onFire; }
    int positionCell() const;
    void freeze(float seconds);
    float remainingPath() const;

    static void drawModel(Gfx& g, EnemyKind kind, float cx, float cy, float facing, float walkPhase, float flash, float alpha);

protected:
    void onDamaged(int amount) override;

private:
    EnemyKind kind;
    int cell;
    int targetCell;
    float speed;
    float radius;
    int reward;
    int damageToCastle;
    float facing;
    float walkPhase;
    float hitFlash;
    float velX;
    float velY;
    float waitTime;
    uint32_t rng;
    float wander;
    int detoursLeft;
    int prevCell;
    float laneX;
    float laneY;
    float aimX;
    float aimY;
    float attackCooldown;
    float freezeTimer;
    float freezeTotal;
    float burnTimer;
    bool inMud;
    bool onFire;
    bool attackingTower;

    float random01();
    int chooseNext();
    void pickAimPoint();
    bool raiderAttack(float dt, Game& game);
};

#endif
