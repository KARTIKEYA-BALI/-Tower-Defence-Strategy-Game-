#include "shop_manager.h"

ShopManager::ShopManager() {
    types.push_back({0, "cannon", "Cannon", 50, 20, 120.0f, 1.0f, 26.0f, 360.0f, ProjectileKind::Cannonball, 12, 1.2f, 8,
                     "Iron shot with a small blast."});
    types.push_back({1, "catapult", "Catapult", 80, 40, 180.0f, 0.5f, 58.0f, 260.0f, ProjectileKind::Boulder, 6, 2.4f, 10,
                     "Lobs boulders that crush groups."});
    types.push_back({2, "crossbow", "Crossbow", 60, 10, 150.0f, 2.0f, 0.0f, 560.0f, ProjectileKind::Bolt, 24, 0.6f, 6,
                     "Rapid bolts, great against runners."});
    types.push_back({3, "archer", "Archer", 40, 8, 140.0f, 1.5f, 0.0f, 470.0f, ProjectileKind::Arrow, 20, 0.8f, 4,
                     "Cheap and steady. Build many."});
}

bool ShopManager::validType(int typeId) const {
    return typeId >= 0 && typeId < typeCount();
}

const TowerType& ShopManager::type(int typeId) const {
    if (!validType(typeId)) return types[0];
    return types[typeId];
}

std::vector<int> ShopManager::allTypes() const {
    std::vector<int> result;
    for (const TowerType& t : types) result.push_back(t.typeId);
    return result;
}
