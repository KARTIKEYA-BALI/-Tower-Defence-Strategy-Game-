#ifndef TOWER_H
#define TOWER_H

#include "ammo_store.h"
#include "entity.h"
#include "shop_manager.h"
#include "targeting_module.h"

class Tower : public Entity {
public:
    static constexpr int MAX_LEVEL = 3;

    Tower(int objectId, const TowerType& type, int cellX, int cellY);

    void update(float dt, Game& game) override;
    void draw(Gfx& g) const override;
    int layer() const override { return 0; }

    int getObjectId() const { return objectId; }
    int getTypeId() const { return type.typeId; }
    const TowerType& getType() const { return type; }
    int getCellX() const { return cellX; }
    int getCellY() const { return cellY; }
    int getLevel() const { return level; }
    const AmmoStore& getAmmo() const { return ammo; }

    int damage() const;
    float range() const;
    float fireRate() const;
    bool canUpgrade() const { return level < MAX_LEVEL; }
    int upgradeCost() const;
    int sellValue() const;
    void upgrade();
    void setLevel(int newLevel);

    void setSelected(bool s) { selected = s; }

    static int upgradeCostFor(const TowerType& type, int level);
    static int damageFor(const TowerType& type, int level);
    static float rangeFor(const TowerType& type, int level);
    static float fireRateFor(const TowerType& type, int level);
    static void drawModel(Gfx& g, int typeId, float cx, float cy, float scale, float aim, int level, float recoil,
                          bool loaded);

private:
    int objectId;
    TowerType type;
    int cellX;
    int cellY;
    int level;
    int invested;
    AmmoStore ammo;
    TargetingModule targeting;
    float cooldown;
    float regenTimer;
    float aim;
    float recoil;
    bool selected;

    void fire(Game& game, const Enemy& target);
};

#endif
