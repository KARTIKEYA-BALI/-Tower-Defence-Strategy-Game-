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
    int health;
    int startScore;
    std::string blurb;
};

class ShopManager {
public:
    ShopManager();

    bool registerWithEngine();
    void shutdownEngineShop();

    int typeCount() const { return static_cast<int>(types.size()); }
    const TowerType& type(int typeId) const;
    bool validType(int typeId) const;

    std::vector<int> allTypes() const;
    std::vector<int> search(const std::string& prefix) const;
    std::vector<int> searchIds(const std::string& prefix) const;
    bool addSearchName(const std::string& name, int id, int score);
    void recordPurchase(int typeId);
    void recordName(const std::string& name);

    static bool allowedSearchChar(char c);
    static constexpr int MAX_SEARCH_LENGTH = 14;
    static constexpr int MAX_SEARCH_IDS = 16;

private:
    std::vector<TowerType> types;
};

#endif
