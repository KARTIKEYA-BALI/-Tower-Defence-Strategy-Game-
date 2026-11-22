#include "troop.h"
#include "cells.h"
#include "engine_core.h"
#include "enemy.h"
#include "game.h"
#include "projectile.h"
#include <algorithm>
#include <cmath>
#include <memory>

namespace {

const float PI_F = 3.14159265358979f;

float wrapAngle(float a) {
    while (a > PI_F) a -= 2.0f * PI_F;
    while (a < -PI_F) a += 2.0f * PI_F;
    return a;
}

float turnToward(float from, float to, float maxStep) {
    float diff = wrapAngle(to - from);
    if (std::fabs(diff) <= maxStep) return to;
    return wrapAngle(from + (diff > 0.0f ? maxStep : -maxStep));
}

SDL_FPoint rotate(float cx, float cy, float lx, float ly, float a) {
    return {cx + lx * std::cos(a) - ly * std::sin(a), cy + lx * std::sin(a) + ly * std::cos(a)};
}

}

Troop::Troop(int postCell)
    : Entity(cellCentreX(postCell), cellCentreY(postCell), HEALTH), post(postCell), cell(postCell), targetId(-1),
      mode(TroopMode::Guard), planGoal(-1), replanTimer(0.0f), heading(PI_F * 0.25f), aim(PI_F * 0.25f), cooldown(0.5f),
      recoil(0.0f), hitFlash(0.0f), wheel(0.0f), regenCarry(0.0f), selected(false) {
}

const char* Troop::modeName() const {
    switch (mode) {
    case TroopMode::Guard:
        return "Guarding its post";
    case TroopMode::Hunt:
        return "Hunting a Raider";
    case TroopMode::Fight:
        return "Fighting nearby";
    case TroopMode::Return:
        return "Driving back";
    }
    return "";
}

int Troop::positionCell() const {
    int cx = std::clamp(static_cast<int>(x / CELL_SIZE), 0, MAP_W - 1);
    int cy = std::clamp(static_cast<int>(y / CELL_SIZE), 0, MAP_H - 1);
    return cellIndex(cx, cy);
}

void Troop::onDamaged(int) {
    hitFlash = 0.18f;
}

Enemy* Troop::chooseTarget(Game& game) {
    Enemy* best = nullptr;
    float bestD = 0.0f;
    for (Enemy* e : game.enemyList()) {
        if (!e->isAlive() || !e->isAttackingTower()) continue;
        float dx = e->getX() - x;
        float dy = e->getY() - y;
        float d = dx * dx + dy * dy;
        if (!best || d < bestD) {
            best = e;
            bestD = d;
        }
    }
    if (best) {
        mode = TroopMode::Hunt;
        return best;
    }
    const float px = cellCentreX(post);
    const float py = cellCentreY(post);
    for (Enemy* e : game.enemyList()) {
        if (!e->isAlive()) continue;
        float ex = e->getX() - px;
        float ey = e->getY() - py;
        if (ex * ex + ey * ey > GUARD_RADIUS * GUARD_RADIUS) continue;
        float dx = e->getX() - x;
        float dy = e->getY() - y;
        float d = dx * dx + dy * dy;
        if (!best || d < bestD) {
            best = e;
            bestD = d;
        }
    }
    if (best) mode = TroopMode::Fight;
    return best;
}

void Troop::moveToward(int goalCell, float dt) {
    if (goalCell != planGoal || replanTimer <= 0.0f || path.empty() || path[0] != cell) {
        int buffer[MAP_W * MAP_H];
        int n = engine_walk_path(cell, goalCell, buffer, MAP_W * MAP_H);
        path.assign(buffer, buffer + std::max(0, n));
        planGoal = goalCell;
        replanTimer = REPLAN;
    }
    int next = path.size() > 1 ? path[1] : cell;
    float tx = cellCentreX(next);
    float ty = cellCentreY(next);
    float dx = tx - x;
    float dy = ty - y;
    float d = std::sqrt(dx * dx + dy * dy);
    float step = SPEED * dt;
    if (d < 0.01f) {
        if (next != cell) {
            cell = next;
            path.erase(path.begin());
        }
        return;
    }
    heading = turnToward(heading, std::atan2(dy, dx), 7.0f * dt);
    wheel += step * 0.12f;
    if (d <= step) {
        x = tx;
        y = ty;
        if (next != cell) {
            cell = next;
            path.erase(path.begin());
        }
    } else {
        x += dx / d * step;
        y += dy / d * step;
    }
}

void Troop::fire(Game& game, const Enemy& target) {
    recoil = 1.0f;
    cooldown = RELOAD;
    float mx = x + std::cos(aim) * 16.0f;
    float my = y + std::sin(aim) * 16.0f;
    game.addEntity(std::make_unique<Projectile>(ProjectileKind::Cannonball, mx, my, target.getId(), target.getX(),
                                                target.getY(), DAMAGE, SPLASH, SHOT_SPEED));
    game.fx().flash(mx, my, rgba(255, 210, 120));
    game.fx().smoke(mx, my, 3);
}

void Troop::update(float dt, Game& game) {
    cooldown = std::max(0.0f, cooldown - dt);
    recoil = std::max(0.0f, recoil - dt * 3.0f);
    hitFlash = std::max(0.0f, hitFlash - dt);
    replanTimer -= dt;

    Enemy* target = chooseTarget(game);
    if (target) {
        targetId = target->getId();
        float dx = target->getX() - x;
        float dy = target->getY() - y;
        float want = std::atan2(dy, dx);
        aim = turnToward(aim, want, 6.0f * dt);
        if (dx * dx + dy * dy <= RANGE * RANGE) {
            if (cooldown <= 0.0f && std::fabs(wrapAngle(want - aim)) < 0.3f) fire(game, *target);
            moveToward(cell, dt);
        } else {
            moveToward(target->positionCell(), dt);
        }
        return;
    }
    targetId = -1;
    if (cell != post || std::fabs(x - cellCentreX(post)) > 0.5f || std::fabs(y - cellCentreY(post)) > 0.5f) {
        mode = TroopMode::Return;
        moveToward(post, dt);
        return;
    }
    mode = TroopMode::Guard;
    aim = turnToward(aim, heading, 2.0f * dt);
    if (getHealth() < getMaxHealth()) {
        regenCarry += REGEN * dt;
        int whole = static_cast<int>(regenCarry);
        if (whole > 0) {
            heal(whole);
            regenCarry -= whole;
        }
    }
}

void Troop::drawModel(Gfx& g, float wx, float wy, float s, float heading, float aim, float wheel, float recoil) {
    g.useGround(0.0f);
    g.glow(wx + 4.0f * s, wy + 4.0f * s, 24.0f * s, 24.0f * s, rgba(0, 0, 0, 130));

    const float bodyL = 15.0f * s;
    const float bodyW = 10.0f * s;
    const float bodyH = 9.0f * s;
    const float lift = 5.0f * s;
    SDL_FPoint corners[4] = {rotate(wx, wy, -bodyL, -bodyW, heading), rotate(wx, wy, bodyL, -bodyW, heading),
                             rotate(wx, wy, bodyL, bodyW, heading), rotate(wx, wy, -bodyL, bodyW, heading)};

    for (int side = -1; side <= 1; side += 2) {
        for (int axle = -1; axle <= 1; axle += 2) {
            SDL_FPoint w = rotate(wx, wy, axle * 9.0f * s, side * (bodyW + 1.5f * s), heading);
            SDL_FPoint c = g.project(w.x, w.y, 6.0f * s);
            g.useScreen();
            g.fillEllipse(c.x, c.y, 6.5f * s, 6.5f * s, rgba(58, 38, 24));
            g.fillEllipse(c.x, c.y, 4.6f * s, 4.6f * s, pal::wood);
            for (int k = 0; k < 3; k++) {
                float a = wheel + k * PI_F / 3.0f;
                g.line(c.x - std::cos(a) * 4.5f * s, c.y - std::sin(a) * 4.5f * s, c.x + std::cos(a) * 4.5f * s,
                       c.y + std::sin(a) * 4.5f * s, 1.0f * s, pal::woodDark);
            }
            g.fillCircle(c.x, c.y, 1.4f * s, pal::iron);
        }
    }

    g.useScreen();
    for (int i = 0; i < 4; i++) {
        const SDL_FPoint& a = corners[i];
        const SDL_FPoint& b = corners[(i + 1) % 4];
        float mx = (a.x + b.x) * 0.5f - wx;
        float my = (a.y + b.y) * 0.5f - wy;
        if (!g.facesViewer(mx, my)) continue;
        float len = std::sqrt(mx * mx + my * my);
        float lit = std::clamp(0.8f + 0.3f * (my - mx) / (len * 1.4142f), 0.5f, 1.1f);
        SDL_FPoint b0 = g.project(a.x, a.y, lift);
        SDL_FPoint b1 = g.project(b.x, b.y, lift);
        SDL_FPoint t0 = g.project(a.x, a.y, lift + bodyH);
        SDL_FPoint t1 = g.project(b.x, b.y, lift + bodyH);
        g.fillPolygonColors({t0, t1, b1, b0}, {shade(pal::wood, lit * 1.08f), shade(pal::wood, lit * 1.08f),
                                               shade(pal::woodDark, lit), shade(pal::woodDark, lit)});
        SDL_FPoint m0 = g.project(a.x, a.y, lift + bodyH * 0.5f);
        SDL_FPoint m1 = g.project(b.x, b.y, lift + bodyH * 0.5f);
        g.line(m0.x, m0.y, m1.x, m1.y, 1.0f, withAlpha(pal::woodDark, 160));
    }
    g.useGround(lift + bodyH);
    std::vector<SDL_FPoint> top(corners, corners + 4);
    g.fillPolygon(top, shade(pal::wood, 1.18f));
    for (int k = -1; k <= 1; k++) {
        SDL_FPoint a = rotate(wx, wy, -bodyL, k * 4.0f * s, heading);
        SDL_FPoint b = rotate(wx, wy, bodyL, k * 4.0f * s, heading);
        g.line(a.x, a.y, b.x, b.y, 0.8f * s, withAlpha(pal::woodDark, 150));
    }

    g.useGround(lift + bodyH + 3.0f * s);
    float back = recoil * 4.0f * s;
    SDL_FPoint b0 = rotate(wx, wy, -4.0f * s - back, -4.2f * s, aim);
    SDL_FPoint b1 = rotate(wx, wy, 20.0f * s - back, -4.2f * s, aim);
    SDL_FPoint b2 = rotate(wx, wy, 20.0f * s - back, 4.2f * s, aim);
    SDL_FPoint b3 = rotate(wx, wy, -4.0f * s - back, 4.2f * s, aim);
    g.fillPolygon({b0, b1, b2, b3}, shade(pal::iron, 0.9f));
    SDL_FPoint h0 = rotate(wx, wy, -4.0f * s - back, -4.2f * s, aim);
    SDL_FPoint h1 = rotate(wx, wy, 20.0f * s - back, -4.2f * s, aim);
    SDL_FPoint h2 = rotate(wx, wy, 20.0f * s - back, -1.6f * s, aim);
    SDL_FPoint h3 = rotate(wx, wy, -4.0f * s - back, -1.6f * s, aim);
    g.fillPolygon({h0, h1, h2, h3}, shade(pal::iron, 1.4f));
    SDL_FPoint m0 = rotate(wx, wy, 16.0f * s - back, -5.4f * s, aim);
    SDL_FPoint m1 = rotate(wx, wy, 21.0f * s - back, -5.4f * s, aim);
    SDL_FPoint m2 = rotate(wx, wy, 21.0f * s - back, 5.4f * s, aim);
    SDL_FPoint m3 = rotate(wx, wy, 16.0f * s - back, 5.4f * s, aim);
    g.fillPolygon({m0, m1, m2, m3}, shade(pal::iron, 0.7f));
    g.fillCircle(wx, wy, 7.0f * s, shade(pal::iron, 0.75f));
    g.fillCircleGradient(wx - 1.0f * s, wy - 1.0f * s, 5.6f * s, shade(pal::iron, 1.6f), pal::iron);
    g.useScreen();
}

void Troop::drawIcon(Gfx& g, float cx, float cy, float scale) {
    IsoView saved = g.camera();
    float hw = 26.0f * scale;
    g.setCamera({cx, cy + 4.0f * scale, hw, hw * 0.6f, hw * 2.0f / CELL_SIZE, false});
    drawModel(g, 0.0f, 0.0f, 1.0f, PI_F * 0.25f, PI_F * 0.15f + 0.4f * std::sin(g.time() * 1.3f), 0.0f, 0.0f);
    g.setCamera(saved);
    g.useScreen();
}

void Troop::draw(Gfx& g) const {
    if (selected) {
        float pulse = 0.5f + 0.5f * std::sin(g.time() * 5.0f);
        g.useGround(0.0f);
        g.strokeCircle(x, y, 24.0f + pulse * 1.5f, 2.4f, withAlpha(pal::supply, 230));
        g.strokeCircle(cellCentreX(post), cellCentreY(post), GUARD_RADIUS, 1.4f, withAlpha(pal::supply, 90));
    }
    drawModel(g, x, y, 1.25f, heading, aim, wheel, recoil);
    g.useBillboard(x, y, 0.0f);
    if (hitFlash > 0.0f) {
        g.glow(x, y - 14.0f, 24.0f, 18.0f, rgba(255, 120, 80, static_cast<int>(170 * std::min(1.0f, hitFlash / 0.18f))));
    }
    float w = 30.0f;
    float bx = x - w * 0.5f;
    float by = y - 46.0f;
    float f = healthFraction();
    g.fillRoundRect(bx - 1.0f, by - 1.0f, w + 2.0f, 6.0f, 3.0f, rgba(10, 10, 14, 210));
    g.fillRoundRect(bx, by, std::max(2.0f, w * f), 4.0f, 2.0f, mix(pal::danger, pal::supply, f));
    if (mode == TroopMode::Hunt) {
        float blink = 0.5f + 0.5f * std::sin(g.time() * 8.0f);
        g.fillCircle(x + 20.0f, y - 44.0f, 5.5f, withAlpha(pal::danger, static_cast<int>(160 + 90 * blink)));
        g.textCentered(FontId::SmallBold, "!", x + 20.0f, y - 44.5f, pal::uiText);
    }
    g.useScreen();
}
