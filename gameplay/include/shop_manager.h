#ifndef SHOP_MANAGER_H
#define SHOP_MANAGER_H

#include <string>
#include <vector>

enum class ProjectileKind { Cannonball, Boulder, Bolt, Arrow };

struct TowerType {
    int typeId;
    std::string name;
    std::string title;
    int cost;
    int damage;
    float range;
    float fireRate;
    float splash;
    float projectileSpeed;
    ProjectileKind projectile;
    int ammoMax;
    float ammoRegenSeconds;
    int startScore;
    std::string blurb;
};

class ShopManager {
public:
    ShopManager();

    int typeCount() const { return static_cast<int>(types.size()); }
    const TowerType& type(int typeId) const;
    bool validType(int typeId) const;

    std::vector<int> allTypes() const;

private:
    std::vector<TowerType> types;
};

#endif
