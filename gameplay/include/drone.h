#ifndef DRONE_H
#define DRONE_H

#include "entity.h"
#include "gfx.h"
#include <vector>

enum class DroneKind { Ammo, Repair };

class Drone : public Entity {
public:
    static constexpr float SPEED = 120.0f;
    static constexpr float ALTITUDE = 16.0f;
    static constexpr int HEALTH = 30;

    Drone(DroneKind kind, int homeCell, int towerEntityId, int towerCell);

    void update(float dt, Game& game) override;
    void draw(Gfx& g) const override;
    int layer() const override { return 3; }

    DroneKind getKind() const { return kind; }
    int getTowerId() const { return towerId; }
    bool isOutbound() const { return outbound; }
    const std::vector<int>& plannedPath() const { return plan; }

    static void drawModel(Gfx& g, DroneKind kind, float cx, float cy, float spin, bool carrying, float flash);

protected:
    void onDamaged(int amount) override;

private:
    DroneKind kind;
    int home;
    int towerId;
    int towerCell;
    int cell;
    int nextCell;
    bool outbound;
    float spin;
    float hitFlash;
    std::vector<int> plan;

    void replan(int destination);
};

#endif
