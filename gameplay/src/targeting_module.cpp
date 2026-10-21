#include "targeting_module.h"
#include "enemy.h"
#include "game.h"

Enemy* TargetingModule::pickTarget(const Game& game, float x, float y, float range) const {
    Enemy* best = nullptr;
    float bestPath = 0.0f;
    for (Enemy* e : game.enemyList()) {
        if (!e->isAlive()) continue;
        float dx = e->getX() - x;
        float dy = e->getY() - y;
        float reach = range + e->getRadius() * 0.5f;
        if (dx * dx + dy * dy > reach * reach) continue;
        float path = e->remainingPath();
        if (!best || path < bestPath) {
            best = e;
            bestPath = path;
        }
    }
    return best;
}
