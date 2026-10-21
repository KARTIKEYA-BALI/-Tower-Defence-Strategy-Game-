#ifndef ENEMY_H
#define ENEMY_H

#include "entity.h"
#include "gfx.h"

enum class EnemyKind { Grunt, Runner, Brute, Warlord };

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
};

#endif
