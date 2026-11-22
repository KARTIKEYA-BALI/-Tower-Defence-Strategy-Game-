#include "drone.h"
#include "cells.h"
#include "engine_core.h"
#include "game.h"
#include "tower.h"
#include <algorithm>
#include <cmath>

namespace {

const float PI_F = 3.14159265358979f;

Color bodyColour(DroneKind kind) {
    return kind == DroneKind::Ammo ? pal::supply : pal::good;
}

}

Drone::Drone(DroneKind k, int homeCell, int towerEntityId, int targetCell)
    : Entity(cellCentreX(homeCell), cellCentreY(homeCell), HEALTH), kind(k), home(homeCell), towerId(towerEntityId),
      towerCell(targetCell), cell(homeCell), nextCell(homeCell), outbound(true), spin(0.0f), hitFlash(0.0f) {
    replan(towerCell);
}

void Drone::onDamaged(int) {
    hitFlash = 0.15f;
}

void Drone::replan(int destination) {
    plan.assign(MAP_W * MAP_H, 0);
    int len = engine_drone_path(cell, destination, plan.data(), static_cast<int>(plan.size()));
    plan.resize(static_cast<size_t>(std::max(0, len)));
    nextCell = plan.size() > 1 ? plan[1] : cell;
}

void Drone::update(float dt, Game& game) {
    spin += dt * 30.0f;
    hitFlash = std::max(0.0f, hitFlash - dt);

    Tower* tower = outbound ? game.findTower(towerId) : nullptr;
    if (outbound && !tower) {
        outbound = false;
        replan(home);
    }

    float tx = cellCentreX(nextCell);
    float ty = cellCentreY(nextCell);
    float dx = tx - x;
    float dy = ty - y;
    float dist = std::sqrt(dx * dx + dy * dy);
    float step = SPEED * dt;
    if (dist > step) {
        x += dx / dist * step;
        y += dy / dist * step;
        return;
    }

    x = tx;
    y = ty;
    cell = nextCell;
    if (outbound && cell == towerCell) {
        game.deliverDrone(*this, *tower);
        outbound = false;
    } else if (!outbound && cell == home) {
        destroy();
        return;
    }
    replan(outbound ? towerCell : home);
}

void Drone::drawModel(Gfx& g, DroneKind kind, float cx, float cy, float spinAngle, bool carrying, float flash) {
    Color body = bodyColour(kind);
    const float arm = 10.0f;
    for (int i = 0; i < 4; i++) {
        float a = PI_F * 0.25f + i * PI_F * 0.5f;
        float rx = cx + std::cos(a) * arm;
        float ry = cy + std::sin(a) * arm * 0.8f;
        g.line(cx, cy, rx, ry, 2.0f, pal::iron);
        g.fillCircle(rx, ry, 4.6f, rgba(30, 34, 44, 170));
        float s = spinAngle + i;
        g.line(rx - std::cos(s) * 5.0f, ry - std::sin(s) * 2.0f, rx + std::cos(s) * 5.0f, ry + std::sin(s) * 2.0f, 1.4f,
               rgba(220, 230, 240, 200));
    }
    g.fillCircle(cx, cy, 6.6f, shade(body, 0.55f));
    g.fillCircleGradient(cx - 0.6f, cy - 0.8f, 5.4f, shade(body, 1.3f), body);
    if (carrying) {
        if (kind == DroneKind::Ammo) {
            g.fillRoundRect(cx - 4.5f, cy + 5.0f, 9.0f, 6.0f, 1.5f, pal::wood);
            g.fillRect(cx - 4.5f, cy + 7.2f, 9.0f, 1.2f, pal::woodDark);
        } else {
            g.fillRoundRect(cx - 4.5f, cy + 5.0f, 9.0f, 7.0f, 1.5f, rgba(236, 236, 240));
            g.fillRect(cx - 0.8f, cy + 6.0f, 1.6f, 5.0f, pal::danger);
            g.fillRect(cx - 2.6f, cy + 7.7f, 5.2f, 1.6f, pal::danger);
        }
    }
    if (flash > 0.0f) g.fillCircle(cx, cy, 8.0f, rgba(255, 255, 255, static_cast<int>(220 * std::min(1.0f, flash / 0.15f))));
}

void Drone::draw(Gfx& g) const {
    g.useGround(0.0f);
    if (plan.size() > 1) {
        Color c = withAlpha(shade(bodyColour(kind), 1.2f), 120);
        float px = x;
        float py = y;
        for (size_t i = 1; i < plan.size(); i++) {
            float qx = cellCentreX(plan[i]);
            float qy = cellCentreY(plan[i]);
            float len = std::sqrt((qx - px) * (qx - px) + (qy - py) * (qy - py));
            for (float d = 4.0f; d < len; d += 9.0f) {
                g.fillCircle(px + (qx - px) * d / len, py + (qy - py) * d / len, 1.5f, c);
            }
            px = qx;
            py = qy;
        }
        g.strokeCircle(px, py, 7.0f, 1.2f, c);
    }
    float bob = std::sin(spin * 0.15f) * 1.5f;
    g.glow(x, y, 13.0f, 13.0f, rgba(0, 0, 0, 100));
    g.useBillboard(x, y, ALTITUDE * 2.4f + bob);
    drawModel(g, kind, x, y, spin, outbound, hitFlash);
    if (getHealth() < getMaxHealth()) {
        float w = 20.0f;
        g.fillRoundRect(x - w * 0.5f - 1.0f, y - 15.0f, w + 2.0f, 5.0f, 2.5f, rgba(10, 10, 14, 200));
        g.fillRoundRect(x - w * 0.5f, y - 14.0f, std::max(2.0f, w * healthFraction()), 3.0f, 1.5f,
                        mix(pal::danger, pal::good, healthFraction()));
    }
    g.useScreen();
}
