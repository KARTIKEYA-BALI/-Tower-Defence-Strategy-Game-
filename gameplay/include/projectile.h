#ifndef PROJECTILE_H
#define PROJECTILE_H

#include "entity.h"
#include "shop_manager.h"

class Projectile : public Entity {
public:
    Projectile(ProjectileKind kind, float x, float y, int targetId, float targetX, float targetY, int damage,
               float splash, float speed);

    void update(float dt, Game& game) override;
    void draw(Gfx& g) const override;
    int layer() const override { return 2; }

private:
    ProjectileKind kind;
    int targetId;
    float startX;
    float startY;
    float targetX;
    float targetY;
    int damage;
    float splash;
    float speed;
    float flightTime;
    float elapsed;
    float height;
    float dirX;
    float dirY;
    float trailTimer;

    void impact(Game& game);
};

#endif
