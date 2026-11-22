#include "enemy.h"
#include "cells.h"
#include "drone.h"
#include "engine_core.h"
#include "game.h"
#include "spell_book.h"
#include "tower.h"
#include "troop.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace {

const float PI_F = 3.14159265358979f;

const EnemyStats STATS[] = {
    {"Grunt", 60, 55.0f, 6, 5, 11.0f, {178, 64, 58, 255}, {86, 30, 28, 255}},
    {"Runner", 34, 96.0f, 5, 4, 9.0f, {150, 98, 196, 255}, {70, 44, 104, 255}},
    {"Brute", 210, 36.0f, 15, 12, 15.0f, {108, 116, 132, 255}, {46, 50, 62, 255}},
    {"Warlord", 1300, 30.0f, 90, 40, 19.0f, {140, 32, 44, 255}, {60, 14, 20, 255}},
    {"Raider", 95, 48.0f, 12, 6, 12.0f, {204, 116, 52, 255}, {96, 50, 22, 255}},
};

const Color RAIDER_FIRE = {255, 150, 50, 255};

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
      hitFlash(0.0f), velX(0.0f), velY(0.0f), waitTime(0.0f), rng(1u), wander(0.0f), detoursLeft(0), prevCell(-1),
      laneX(0.0f), laneY(0.0f), aimX(x), aimY(y), attackCooldown(RAIDER_RELOAD), freezeTimer(0.0f), freezeTotal(0.0f),
      burnTimer(0.0f), inMud(false), onFire(false), attackingTower(false) {
    rng = (static_cast<uint32_t>(getId()) * 2654435761u) ^ (static_cast<uint32_t>(std::rand()) * 40503u);
    if (rng == 0u) rng = 1u;
    speed *= 0.88f + 0.24f * random01();
    wander = 0.08f + 0.22f * random01();
    detoursLeft = static_cast<int>(random01() * 4.0f);
    laneX = (random01() - 0.5f) * 20.0f;
    laneY = (random01() - 0.5f) * 20.0f;
    int next = chooseNext();
    if (next >= 0 && next != cell) {
        targetCell = next;
        pickAimPoint();
    }
}

float Enemy::random01() {
    rng ^= rng << 13;
    rng ^= rng >> 17;
    rng ^= rng << 5;
    return static_cast<float>(rng & 0xFFFFFFu) / 16777216.0f;
}

int Enemy::chooseNext() {
    int here = engine_distance_to_goal(cell);
    if (here == 0) return cell;
    if (here < 0) return engine_next_step(cell);

    const int dx[4] = {1, -1, 0, 0};
    const int dy[4] = {0, 0, 1, -1};
    int down[4];
    int up[4];
    int nd = 0;
    int nu = 0;
    for (int k = 0; k < 4; k++) {
        int nx = cellX(cell) + dx[k];
        int ny = cellY(cell) + dy[k];
        if (!insideMap(nx, ny)) continue;
        int n = cellIndex(nx, ny);
        int d = engine_distance_to_goal(n);
        if (d < 0) continue;
        if (d < here) down[nd++] = n;
        else if (d > here && n != prevCell) up[nu++] = n;
    }

    if (nu > 0 && detoursLeft > 0 && random01() < wander) {
        detoursLeft--;
        return up[static_cast<int>(random01() * nu) % nu];
    }
    if (nd == 0) return engine_next_step(cell);
    if (nd > 1) {
        for (int i = 0; i < nd; i++) {
            if (down[i] == prevCell) {
                down[i] = down[--nd];
                break;
            }
        }
    }
    return down[static_cast<int>(random01() * nd) % nd];
}

void Enemy::pickAimPoint() {
    aimX = cellCentreX(targetCell);
    aimY = cellCentreY(targetCell);
    if (targetCell == engine_goal_cell()) return;
    aimX += laneX + (random01() - 0.5f) * 8.0f;
    aimY += laneY + (random01() - 0.5f) * 8.0f;
}

bool Enemy::raiderAttack(float dt, Game& game) {
    attackCooldown = std::max(0.0f, attackCooldown - dt);
    const float reach = RAIDER_RANGE * RAIDER_RANGE;

    Drone* drone = nullptr;
    float best = reach;
    for (Drone* d : game.droneList()) {
        if (!d->isAlive()) continue;
        float ddx = d->getX() - x;
        float ddy = d->getY() - y;
        float d2 = ddx * ddx + ddy * ddy;
        if (d2 <= best) {
            best = d2;
            drone = d;
        }
    }
    if (drone) {
        facing = std::atan2(drone->getY() - y, drone->getX() - x);
        if (attackCooldown <= 0.0f) {
            attackCooldown = RAIDER_RELOAD;
            game.fx().streak(x, y, drone->getX(), drone->getY() - Drone::ALTITUDE, RAIDER_FIRE);
            game.damageDrone(*drone, RAIDER_DAMAGE);
        }
        return false;
    }

    Troop* troop = nullptr;
    best = reach;
    for (Troop* t : game.troopList()) {
        if (!t->isAlive()) continue;
        float tdx = t->getX() - x;
        float tdy = t->getY() - y;
        float d2 = tdx * tdx + tdy * tdy;
        if (d2 <= best) {
            best = d2;
            troop = t;
        }
    }
    if (troop) {
        facing = std::atan2(troop->getY() - y, troop->getX() - x);
        if (attackCooldown <= 0.0f) {
            attackCooldown = RAIDER_RELOAD;
            game.fx().streak(x, y, troop->getX(), troop->getY(), RAIDER_FIRE);
            game.damageTroop(*troop, RAIDER_DAMAGE);
        }
        return false;
    }

    Tower* tower = nullptr;
    best = reach;
    for (Tower* t : game.towerList()) {
        if (!t->isAlive()) continue;
        float tdx = t->getX() - x;
        float tdy = t->getY() - y;
        float d2 = tdx * tdx + tdy * tdy;
        if (d2 <= best) {
            best = d2;
            tower = t;
        }
    }
    if (!tower) return false;
    attackingTower = true;
    facing = std::atan2(tower->getY() - y, tower->getX() - x);
    if (attackCooldown <= 0.0f) {
        attackCooldown = RAIDER_RELOAD;
        game.fx().streak(x, y, tower->getX(), tower->getY(), RAIDER_FIRE);
        game.damageTower(*tower, RAIDER_DAMAGE);
    }
    return true;
}

float Enemy::remainingPath() const {
    int d = engine_distance_to_goal(targetCell);
    if (d < 0) d = MAP_W * MAP_H;
    float dx = aimX - x;
    float dy = aimY - y;
    return d * CELL_SIZE + std::sqrt(dx * dx + dy * dy);
}

void Enemy::onDamaged(int) {
    hitFlash = 0.12f;
}

int Enemy::positionCell() const {
    int cx = std::clamp(static_cast<int>(x / CELL_SIZE), 0, MAP_W - 1);
    int cy = std::clamp(static_cast<int>(y / CELL_SIZE), 0, MAP_H - 1);
    return cellIndex(cx, cy);
}

void Enemy::freeze(float seconds) {
    if (seconds <= 0.0f) return;
    if (kind == EnemyKind::Warlord) seconds *= SpellBook::BOSS_FREEZE_SCALE;
    if (seconds > freezeTimer) {
        freezeTimer = seconds;
        freezeTotal = seconds;
    }
}

void Enemy::update(float dt, Game& game) {
    hitFlash = std::max(0.0f, hitFlash - dt);
    velX = 0.0f;
    velY = 0.0f;
    attackingTower = false;

    int here = positionCell();
    inMud = engine_zone_at(here, ZONE_MUD) > 0;
    onFire = engine_zone_at(here, ZONE_FIRE) > 0;
    if (onFire) {
        burnTimer -= dt;
        if (burnTimer <= 0.0f) {
            burnTimer += SpellBook::FIRE_TICK;
            game.damageEnemy(*this, SpellBook::FIRE_TICK_DAMAGE);
            if (!isAlive()) return;
        }
    } else {
        burnTimer = 0.0f;
    }

    if (freezeTimer > 0.0f) {
        freezeTimer = std::max(0.0f, freezeTimer - dt);
        return;
    }

    if (kind == EnemyKind::Raider && raiderAttack(dt, game)) return;

    if (targetCell == cell) {
        if (cell == engine_goal_cell()) {
            game.damageCastle(damageToCastle, x, y);
            destroy();
            return;
        }
        int next = chooseNext();
        if (next < 0 || next == cell) {
            waitTime += dt;
            return;
        }
        targetCell = next;
        pickAimPoint();
    }

    float tx = aimX;
    float ty = aimY;
    float dx = tx - x;
    float dy = ty - y;
    float dist = std::sqrt(dx * dx + dy * dy);
    float pace = speed * (inMud ? SpellBook::MUD_SLOW : 1.0f);
    float step = pace * dt;

    if (dist > 0.001f) {
        float want = std::atan2(dy, dx);
        float diff = wrapAngle(want - facing);
        facing = wrapAngle(facing + diff * std::min(1.0f, dt * 12.0f));
        velX = dx / dist * pace;
        velY = dy / dist * pace;
    }
    walkPhase += dt * pace * 0.16f;

    if (dist <= step) {
        x = tx;
        y = ty;
        prevCell = cell;
        cell = targetCell;
        if (cell == engine_goal_cell()) {
            game.damageCastle(damageToCastle, x, y);
            destroy();
            return;
        }
        int next = chooseNext();
        targetCell = (next >= 0) ? next : cell;
        if (targetCell != cell) pickAimPoint();
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
    } else if (kind == EnemyKind::Raider) {
        g.strokeArc(cx, by, r * 0.8f, -PI_F * 0.92f, -PI_F * 0.08f, r * 0.3f, rgba(70, 44, 30));
        float hx = cx + perpX * r * 1.05f + dirX * r * 0.2f;
        float hy = by + perpY * r * 1.05f + dirY * r * 0.2f;
        float tipX = hx + dirX * r * 0.45f;
        float tipY = hy + dirY * r * 0.45f - r * 0.65f;
        g.line(hx, hy, tipX, tipY, 2.2f, pal::woodDark);
        float flick = 0.5f + 0.5f * std::sin(g.time() * 13.0f + cx * 0.1f);
        g.glow(tipX, tipY - 2.0f, r * (0.9f + 0.25f * flick), r * (0.9f + 0.25f * flick), rgba(255, 150, 40, 150));
        g.fillCircle(tipX, tipY - 1.5f, r * (0.26f + 0.05f * flick), rgba(255, 210, 90));
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
    g.useGround(0.0f);
    if (kind == EnemyKind::Raider && !isFrozen()) g.strokeCircle(x, y, RAIDER_RANGE, 1.4f, rgba(240, 110, 60, 60));
    if (inMud) {
        for (int i = 0; i < 3; i++) {
            float a = walkPhase * 0.7f + i * 2.1f;
            g.fillEllipse(x + std::cos(a) * radius * 0.9f, y + radius * 0.75f + std::sin(a) * 2.0f, 3.2f, 1.8f,
                          rgba(92, 64, 36, 210));
        }
    }
    SDL_FPoint s0 = g.project(x, y);
    SDL_FPoint s1 = g.project(x + std::cos(facing) * 10.0f, y + std::sin(facing) * 10.0f);
    float screenFacing = std::atan2(s1.y - s0.y, s1.x - s0.x);
    g.useBillboard(x, y, radius * 0.7f, 1.2f);
    drawModel(g, kind, x, y, screenFacing, walkPhase, hitFlash, 1.0f);
    if (onFire) {
        float t = g.time() * 10.0f + getId();
        for (int i = 0; i < 3; i++) {
            float fx = x + (i - 1) * radius * 0.55f;
            float h = radius * (0.9f + 0.35f * std::sin(t + i * 1.9f));
            g.glow(fx, y - radius * 0.2f, radius * 0.5f, h * 0.7f, rgba(255, 120, 30, 120));
            g.fillTriangle({fx - radius * 0.22f, y}, {fx + radius * 0.22f, y}, {fx, y - h}, rgba(255, 170, 60, 200));
        }
    }
    if (isFrozen()) {
        float f = freezeTotal > 0.0f ? std::clamp(freezeTimer / freezeTotal, 0.0f, 1.0f) : 0.0f;
        int a = static_cast<int>(90 + 110 * std::min(1.0f, f * 3.0f));
        float r = radius + 5.0f;
        g.fillRoundRect(x - r, y - r - 2.0f, r * 2.0f, r * 2.0f, r * 0.45f, rgba(170, 220, 255, a));
        g.strokeRoundRect(x - r, y - r - 2.0f, r * 2.0f, r * 2.0f, r * 0.45f, 1.4f, rgba(235, 248, 255, a + 40));
        g.line(x - r * 0.55f, y - r * 0.6f, x - r * 0.1f, y - r * 0.15f, 1.6f, rgba(255, 255, 255, a));
        g.line(x + r * 0.2f, y + r * 0.1f, x + r * 0.5f, y + r * 0.45f, 1.2f, rgba(255, 255, 255, a));
        float bw = r * 2.0f * f;
        g.fillRoundRect(x - r, y + r + 1.0f, r * 2.0f, 3.0f, 1.5f, rgba(10, 20, 30, 160));
        g.fillRoundRect(x - r, y + r + 1.0f, std::max(2.0f, bw), 3.0f, 1.5f, rgba(150, 210, 255));
    }

    if (getHealth() < getMaxHealth()) {
        float w = std::max(22.0f, radius * 2.2f);
        float bx = x - w * 0.5f;
        float by = y - radius - 11.0f;
        g.fillRoundRect(bx - 1.0f, by - 1.0f, w + 2.0f, 6.0f, 3.0f, rgba(10, 10, 14, 200));
        float f = healthFraction();
        Color fill = mix(rgba(236, 70, 58), rgba(250, 196, 72), f);
        g.fillRoundRect(bx, by, std::max(2.0f, w * f), 4.0f, 2.0f, fill);
    }
    g.useScreen();
}
