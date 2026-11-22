#include "shop_manager.h"
#include "engine_core.h"
#include <algorithm>
#include <cctype>

ShopManager::ShopManager() {
    types.push_back({0, "cannon", "Cannon", 50, 20, 120.0f, 1.0f, 26.0f, 360.0f, ProjectileKind::Cannonball, 38, 175, 8,
                     "Iron shot with a small blast."});
    types.push_back({1, "catapult", "Catapult", 80, 40, 180.0f, 0.5f, 58.0f, 260.0f, ProjectileKind::Boulder, 18, 150, 10,
                     "Lobs boulders that crush groups."});
    types.push_back({2, "crossbow", "Crossbow", 60, 10, 150.0f, 2.0f, 0.0f, 560.0f, ProjectileKind::Bolt, 75, 125, 6,
                     "Rapid bolts, great against runners."});
    types.push_back({3, "archer", "Archer", 40, 8, 140.0f, 1.5f, 0.0f, 470.0f, ProjectileKind::Arrow, 60, 100, 4,
                     "Cheap and steady. Build many."});
}

bool ShopManager::registerWithEngine() {
    if (!shop_init()) return false;
    bool ok = true;
    for (const TowerType& t : types) {
        if (!shop_insert(t.name.c_str(), t.typeId, t.startScore)) ok = false;
    }
    return ok;
}

void ShopManager::shutdownEngineShop() {
    shop_shutdown();
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

bool ShopManager::allowedSearchChar(char c) {
    unsigned char u = static_cast<unsigned char>(c);
    return std::isalnum(u) || c == ' ';
}

std::vector<int> ShopManager::searchIds(const std::string& prefix) const {
    int ids[MAX_SEARCH_IDS];
    int n = shop_search(prefix.c_str(), ids, MAX_SEARCH_IDS);
    return std::vector<int>(ids, ids + std::max(0, n));
}

std::vector<int> ShopManager::search(const std::string& prefix) const {
    if (prefix.empty()) return allTypes();
    std::vector<int> result;
    for (int id : searchIds(prefix)) {
        if (validType(id) && static_cast<int>(result.size()) < SHOP_RESULTS) result.push_back(id);
    }
    return result;
}

bool ShopManager::addSearchName(const std::string& name, int id, int score) {
    return shop_insert(name.c_str(), id, score) != 0;
}

void ShopManager::recordName(const std::string& name) {
    shop_record_purchase(name.c_str());
}

void ShopManager::recordPurchase(int typeId) {
    if (validType(typeId)) shop_record_purchase(types[typeId].name.c_str());
}
