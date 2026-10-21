#include "entity.h"
#include <algorithm>

int Entity::nextId = 1;

Entity::Entity(float startX, float startY, int maxHp)
    : x(startX), y(startY), id(nextId++), health(std::max(1, maxHp)), maxHealth(std::max(1, maxHp)), alive(true) {
}

float Entity::healthFraction() const {
    return static_cast<float>(health) / static_cast<float>(maxHealth);
}

bool Entity::takeDamage(int amount) {
    if (!alive || amount <= 0) return false;
    health = std::max(0, health - amount);
    onDamaged(amount);
    if (health == 0) {
        alive = false;
        return true;
    }
    return false;
}

void Entity::destroy() {
    alive = false;
}

void Entity::onDamaged(int) {
}
