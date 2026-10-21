#include "enemy.h"
#include "cells.h"
#include "engine_core.h"
#include "game.h"
#include <algorithm>
#include <cmath>

namespace {

const float PI_F = 3.14159265358979f;

const EnemyStats STATS[] = {
    {"Grunt", 60, 55.0f, 6, 5, 11.0f, {178, 64, 58, 255}, {86, 30, 28, 255}},
    {"Runner", 34, 96.0f, 5, 4, 9.0f, {150, 98, 196, 255}, {70, 44, 104, 255}},
    {"Brute", 210, 36.0f, 15, 12, 15.0f, {108, 116, 132, 255}, {46, 50, 62, 255}},
    {"Warlord", 1300, 30.0f, 90, 40, 19.0f, {140, 32, 44, 255}, {60, 14, 20, 255}},
};

float wrapAngle(float a) {
    while (a > PI_F) a -= 2.0f * PI_F;
    while (a < -PI_F) a += 2.0f * PI_F;
    return a;
}

}

const EnemyStats& enemyStats(EnemyKind kind) {
    return STATS[static_cast<int>(kind)];
}

Enemy::Enemy(EnemyKind k, int spawnCell, float healthScale)
    : Entity(cellCentreX(spawnCell), cellCentreY(spawnCell),
             static_cast<int>(std::lround(enemyStats(k).health * healthScale))),
      kind(k), cell(spawnCell), targetCell(spawnCell), speed(enemyStats(k).speed), radius(enemyStats(k).radius),
      reward(enemyStats(k).reward), damageToCastle(enemyStats(k).damageToCastle), facing(0.0f), walkPhase(0.0f),
      hitFlash(0.0f), velX(0.0f), velY(0.0f), waitTime(0.0f) {
    int next = engine_next_step(cell);
    if (next >= 0) targetCell = next;
}

float Enemy::remainingPath() const {
    int d = engine_distance_to_goal(targetCell);
    if (d < 0) d = MAP_W * MAP_H;
    float dx = cellCentreX(targetCell) - x;
    float dy = cellCentreY(targetCell) - y;
    return d * CELL_SIZE + std::sqrt(dx * dx + dy * dy);
}

void Enemy::onDamaged(int) {
    hitFlash = 0.12f;
}

void Enemy::update(float dt, Game& game) {
    hitFlash = std::max(0.0f, hitFlash - dt);
    velX = 0.0f;
    velY = 0.0f;

    if (targetCell == cell) {
        if (cell == engine_goal_cell()) {
            game.damageCastle(damageToCastle, x, y);
            destroy();
            return;
        }
        int next = engine_next_step(cell);
        if (next < 0 || next == cell) {
            waitTime += dt;
            return;
        }
        targetCell = next;
    }

    float tx = cellCentreX(targetCell);
    float ty = cellCentreY(targetCell);
    float dx = tx - x;
    float dy = ty - y;
    float dist = std::sqrt(dx * dx + dy * dy);
    float step = speed * dt;

    if (dist > 0.001f) {
        float want = std::atan2(dy, dx);
        float diff = wrapAngle(want - facing);
        facing = wrapAngle(facing + diff * std::min(1.0f, dt * 12.0f));
        velX = dx / dist * speed;
        velY = dy / dist * speed;
    }
    walkPhase += dt * speed * 0.16f;

    if (dist <= step) {
        x = tx;
        y = ty;
        cell = targetCell;
        if (cell == engine_goal_cell()) {
            game.damageCastle(damageToCastle, x, y);
            destroy();
            return;
        }
        int next = engine_next_step(cell);
        targetCell = (next >= 0) ? next : cell;
    } else {
        x += dx / dist * step;
        y += dy / dist * step;
    }
}

void Enemy::drawModel(Gfx& g, EnemyKind kind, float cx, float cy, float face, float phase, float flash, float alpha) {
    const EnemyStats& s = enemyStats(kind);
    const float r = s.radius;
    const float prevAlpha = g.alpha();
    g.setAlpha(prevAlpha * alpha);

    float bob = std::fabs(std::sin(phase)) * 1.6f;
    float dirX = std::cos(face);
    float dirY = std::sin(face);
    float perpX = -dirY;
    float perpY = dirX;

    g.glow(cx, cy + r * 0.72f, r * 1.25f, r * 0.55f, rgba(0, 0, 0, 120));

    if (kind == EnemyKind::Warlord) {
        float pulse = 0.5f + 0.5f * std::sin(g.time() * 4.0f);
        g.glow(cx, cy, r * 2.2f, r * 2.2f, rgba(230, 60, 40, static_cast<int>(50 + 40 * pulse)));
    }

    float swing = std::sin(phase) * r * 0.32f;
    for (int side = -1; side <= 1; side += 2) {
        float fx = cx + perpX * r * 0.45f * side + dirX * swing * side;
        float fy = cy + perpY * r * 0.45f * side + dirY * swing * side + r * 0.45f;
        g.fillCircle(fx, fy, r * 0.3f, shade(s.trim, 0.8f));
    }

    float by = cy - bob;

    if (kind == EnemyKind::Warlord) {
        g.fillEllipse(cx - dirX * r * 0.35f, by - dirY * r * 0.35f + r * 0.25f, r * 1.05f, r * 0.9f, shade(s.trim, 1.2f));
    }
    if (kind == EnemyKind::Runner) {
        SDL_FPoint a = {cx - dirX * r * 1.55f, by - dirY * r * 1.55f};
        SDL_FPoint b = {cx + perpX * r * 0.7f, by + perpY * r * 0.7f};
        SDL_FPoint c = {cx - perpX * r * 0.7f, by - perpY * r * 0.7f};
        g.fillTriangle(a, b, c, s.trim);
    }

    g.fillCircle(cx, by, r, shade(s.body, 0.62f));
    g.fillCircle(cx - r * 0.12f, by - r * 0.16f, r * 0.84f, s.body);
    g.glow(cx - r * 0.32f, by - r * 0.38f, r * 0.5f, r * 0.42f, rgba(255, 255, 255, 70));

    if (kind == EnemyKind::Grunt) {
        g.strokeArc(cx, by, r * 0.78f, -PI_F * 0.95f, -PI_F * 0.05f, r * 0.32f, pal::iron);
        g.fillCircle(cx, by - r * 0.92f, r * 0.18f, pal::stoneLight);
    } else if (kind == EnemyKind::Brute) {
        for (int side = -1; side <= 1; side += 2) {
            float px = cx + perpX * r * 0.82f * side;
            float py = by + perpY * r * 0.82f * side;
            g.fillCircle(px, py, r * 0.42f, pal::iron);
            g.fillCircle(px - r * 0.08f, py - r * 0.1f, r * 0.3f, shade(pal::stone, 0.9f));
        }
        for (int side = -1; side <= 1; side += 2) {
            SDL_FPoint base1 = {cx + perpX * r * 0.35f * side - r * 0.12f, by - r * 0.62f};
            SDL_FPoint base2 = {cx + perpX * r * 0.55f * side + r * 0.12f, by - r * 0.55f};
            SDL_FPoint tip = {cx + perpX * r * 0.8f * side, by - r * 1.25f};
            g.fillTriangle(base1, base2, tip, rgba(232, 222, 196));
        }
    } else if (kind == EnemyKind::Warlord) {
        float crownY = by - r * 0.78f;
        for (int i = -1; i <= 1; i++) {
            SDL_FPoint l = {cx + i * r * 0.36f - r * 0.16f, crownY};
            SDL_FPoint rr = {cx + i * r * 0.36f + r * 0.16f, crownY};
            SDL_FPoint tip = {cx + i * r * 0.36f, crownY - r * (i == 0 ? 0.62f : 0.45f)};
            g.fillTriangle(l, rr, tip, pal::gold);
        }
        g.fillRoundRect(cx - r * 0.62f, crownY - r * 0.06f, r * 1.24f, r * 0.26f, r * 0.08f, pal::goldDeep);
    }

    g.strokeCircle(cx, by, r, 1.3f, withAlpha(s.trim, 210));

    Color eye = (kind == EnemyKind::Brute || kind == EnemyKind::Warlord) ? rgba(255, 90, 60) : rgba(255, 236, 180);
    float ex = cx + dirX * r * 0.42f;
    float ey = by + dirY * r * 0.42f - r * 0.05f;
    for (int side = -1; side <= 1; side += 2) {
        g.fillCircle(ex + perpX * r * 0.3f * side, ey + perpY * r * 0.3f * side, std::max(1.6f, r * 0.14f), eye);
    }

    if (flash > 0.0f) {
        g.fillCircle(cx, by, r + 0.5f, rgba(255, 255, 255, static_cast<int>(200 * std::min(1.0f, flash / 0.12f))));
    }

    g.setAlpha(prevAlpha);
}

void Enemy::draw(Gfx& g) const {
    drawModel(g, kind, x, y, facing, walkPhase, hitFlash, 1.0f);

    if (getHealth() < getMaxHealth()) {
        float w = std::max(22.0f, radius * 2.2f);
        float bx = x - w * 0.5f;
        float by = y - radius - 11.0f;
        g.fillRoundRect(bx - 1.0f, by - 1.0f, w + 2.0f, 6.0f, 3.0f, rgba(10, 10, 14, 200));
        float f = healthFraction();
        Color fill = mix(rgba(236, 70, 58), rgba(250, 196, 72), f);
        g.fillRoundRect(bx, by, std::max(2.0f, w * f), 4.0f, 2.0f, fill);
    }
}
