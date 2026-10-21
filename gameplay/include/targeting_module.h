#ifndef TARGETING_MODULE_H
#define TARGETING_MODULE_H

class Enemy;
class Game;

class TargetingModule {
public:
    Enemy* pickTarget(const Game& game, float x, float y, float range) const;
};

#endif
