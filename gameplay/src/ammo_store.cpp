#include "ammo_store.h"
#include <algorithm>

AmmoStore::AmmoStore(int maxAmmo) : count(std::max(1, maxAmmo)), capacity(std::max(1, maxAmmo)) {
}

bool AmmoStore::use() {
    if (count <= 0) return false;
    count--;
    return true;
}

void AmmoStore::refill(int amount) {
    if (amount <= 0) return;
    count = std::min(capacity, count + amount);
}

void AmmoStore::refillFull() {
    count = capacity;
}

void AmmoStore::setMax(int maxAmmo) {
    capacity = std::max(1, maxAmmo);
    count = std::min(count, capacity);
}

float AmmoStore::fraction() const {
    return static_cast<float>(count) / static_cast<float>(capacity);
}
