#include "tower.h"
#include "cells.h"
#include "enemy.h"
#include "game.h"
#include "gfx.h"
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

struct Local {
    float cx;
    float cy;
    float s;
    float c;
    float sn;
    Local(float x, float y, float scale, float angle) : cx(x), cy(y), s(scale), c(std::cos(angle)), sn(std::sin(angle)) {}
    SDL_FPoint p(float lx, float ly) const {
        return {cx + (lx * c - ly * sn) * s, cy + (lx * sn + ly * c) * s};
    }
    std::vector<SDL_FPoint> rect(float x0, float y0, float x1, float y1) const {
        return {p(x0, y0), p(x1, y0), p(x1, y1), p(x0, y1)};
    }
};

void stoneBase(Gfx& g, float cx, float cy, float s) {
    std::vector<SDL_FPoint> outer;
    std::vector<SDL_FPoint> inner;
    for (int i = 0; i < 8; i++) {
        float a = PI_F / 8.0f + i * PI_F / 4.0f;
        outer.push_back({cx + std::cos(a) * 20.0f * s, cy + std::sin(a) * 20.0f * s});
        inner.push_back({cx + std::cos(a) * 15.5f * s, cy + std::sin(a) * 15.5f * s});
    }
    g.fillPolygonGradient(outer, pal::stoneLight, pal::stoneDark);
    g.fillPolygonGradient(inner, shade(pal::stone, 0.86f), shade(pal::stone, 1.06f));
    for (int i = 0; i < 8; i++) {
        float a = i * PI_F / 4.0f;
        float x0 = cx + std::cos(a) * 15.8f * s;
        float y0 = cy + std::sin(a) * 15.8f * s;
        float x1 = cx + std::cos(a) * 19.6f * s;
        float y1 = cy + std::sin(a) * 19.6f * s;
        g.line(x0, y0, x1, y1, 1.0f * s, withAlpha(pal::stoneDark, 150));
    }
}

}

Tower::Tower(int id, const TowerType& t, int cx, int cy)
    : Entity(cx * CELL_SIZE + CELL_SIZE * 0.5f, cy * CELL_SIZE + CELL_SIZE * 0.5f, 1), objectId(id), type(t), cellX(cx),
      cellY(cy), level(1), invested(t.cost), ammo(t.ammoMax), cooldown(0.2f), regenTimer(0.0f), aim(-PI_F * 0.5f),
      recoil(0.0f), selected(false) {
}

int Tower::damageFor(const TowerType& t, int lvl) {
    return static_cast<int>(std::lround(t.damage * (1.0f + 0.35f * (lvl - 1))));
}

float Tower::rangeFor(const TowerType& t, int lvl) {
    return t.range * (1.0f + 0.12f * (lvl - 1));
}

float Tower::fireRateFor(const TowerType& t, int lvl) {
    return t.fireRate * (1.0f + 0.08f * (lvl - 1));
}

int Tower::upgradeCostFor(const TowerType& t, int lvl) {
    return static_cast<int>(std::lround(t.cost * 0.6f * lvl));
}

int Tower::damage() const {
    return damageFor(type, level);
}

float Tower::range() const {
    return rangeFor(type, level);
}

float Tower::fireRate() const {
    return fireRateFor(type, level);
}

int Tower::upgradeCost() const {
    return upgradeCostFor(type, level);
}

int Tower::sellValue() const {
    return invested / 2;
}

void Tower::upgrade() {
    if (!canUpgrade()) return;
    invested += upgradeCost();
    level++;
    ammo.setMax(type.ammoMax + (level - 1) * type.ammoMax / 4);
    ammo.refillFull();
}

void Tower::setLevel(int newLevel) {
    newLevel = std::clamp(newLevel, 1, MAX_LEVEL);
    while (level < newLevel) upgrade();
}

void Tower::update(float dt, Game& game) {
    cooldown = std::max(0.0f, cooldown - dt);
    recoil = std::max(0.0f, recoil - dt * 3.0f);

    if (!ammo.full()) {
        regenTimer += dt;
        if (regenTimer >= type.ammoRegenSeconds) {
            regenTimer = 0.0f;
            ammo.refill(1);
        }
    } else {
        regenTimer = 0.0f;
    }

    Enemy* target = targeting.pickTarget(game, x, y, range());
    if (!target) return;

    float want = std::atan2(target->getY() - y, target->getX() - x);
    float turnRate = (type.projectile == ProjectileKind::Boulder) ? 5.0f : 11.0f;
    float diff = wrapAngle(want - aim);
    float stepTurn = turnRate * dt;
    if (std::fabs(diff) <= stepTurn) aim = want;
    else aim = wrapAngle(aim + (diff > 0 ? stepTurn : -stepTurn));

    if (cooldown <= 0.0f && std::fabs(wrapAngle(want - aim)) < 0.35f) {
        if (ammo.use()) {
            fire(game, *target);
            cooldown = 1.0f / fireRate();
        }
    }
}

void Tower::fire(Game& game, const Enemy& target) {
    recoil = 1.0f;
    float mx = x + std::cos(aim) * 18.0f;
    float my = y + std::sin(aim) * 18.0f;
    float tx = target.getX();
    float ty = target.getY();
    if (type.projectile == ProjectileKind::Boulder) {
        float dx = tx - x;
        float dy = ty - y;
        float flight = std::clamp(std::sqrt(dx * dx + dy * dy) / type.projectileSpeed, 0.55f, 1.0f);
        tx += target.getVelX() * flight;
        ty += target.getVelY() * flight;
        mx = x;
        my = y;
    }
    game.addEntity(std::make_unique<Projectile>(type.projectile, mx, my, target.getId(), tx, ty, damage(), type.splash,
                                                type.projectileSpeed));
    if (type.projectile == ProjectileKind::Cannonball) {
        game.fx().flash(mx, my, rgba(255, 210, 120));
        game.fx().smoke(mx, my, 3);
    } else if (type.projectile == ProjectileKind::Boulder) {
        game.fx().smoke(x, y, 2);
    }
}

void Tower::drawModel(Gfx& g, int typeId, float cx, float cy, float s, float aim, int level, float recoil, bool loaded) {
    g.glow(cx + 2.0f * s, cy + 5.0f * s, 25.0f * s, 16.0f * s, rgba(0, 0, 0, 130));
    Local L(cx, cy, s, aim);

    if (typeId == 3) {
        g.fillCircle(cx, cy, 20.0f * s, pal::woodDark);
        g.fillCircleGradient(cx, cy - 1.0f * s, 17.5f * s, shade(pal::wood, 1.12f), shade(pal::wood, 0.92f));
        g.strokeCircle(cx, cy, 13.0f * s, 1.2f * s, withAlpha(pal::woodDark, 170));
        for (int i = 0; i < 10; i++) {
            float a = i * PI_F / 5.0f;
            g.fillCircle(cx + std::cos(a) * 18.0f * s, cy + std::sin(a) * 18.0f * s, 2.2f * s, shade(pal::wood, 1.25f));
        }
    } else {
        stoneBase(g, cx, cy, s);
    }

    if (typeId == 0) {
        float back = recoil * 4.0f;
        g.fillPolygon(L.rect(-2.0f - back, -5.0f, 21.0f - back, 5.0f), shade(pal::iron, 0.9f));
        g.fillPolygon(L.rect(-2.0f - back, -5.0f, 21.0f - back, -2.2f), shade(pal::iron, 1.35f));
        g.fillPolygon(L.rect(16.0f - back, -6.2f, 22.0f - back, 6.2f), shade(pal::iron, 0.7f));
        g.fillCircle(cx, cy, 11.5f * s, shade(pal::iron, 0.75f));
        g.fillCircleGradient(cx - 1.5f * s, cy - 1.5f * s, 9.5f * s, shade(pal::iron, 1.6f), shade(pal::iron, 1.0f));
        for (int i = 0; i < 4; i++) {
            SDL_FPoint p = L.p(std::cos(i * PI_F * 0.5f + PI_F * 0.25f) * 7.0f, std::sin(i * PI_F * 0.5f + PI_F * 0.25f) * 7.0f);
            g.fillCircle(p.x, p.y, 1.3f * s, pal::stoneLight);
        }
    } else if (typeId == 1) {
        std::vector<SDL_FPoint> deck = L.rect(-14.0f, -12.0f, 14.0f, 12.0f);
        g.fillPolygon(deck, pal::woodDark);
        g.fillPolygon(L.rect(-13.0f, -11.0f, 13.0f, 11.0f), pal::wood);
        for (int i = -1; i <= 1; i++) {
            SDL_FPoint a = L.p(-13.0f, i * 6.0f);
            SDL_FPoint b = L.p(13.0f, i * 6.0f);
            g.line(a.x, a.y, b.x, b.y, 0.9f * s, withAlpha(pal::woodDark, 160));
        }
        g.fillPolygon(L.rect(-13.0f, -11.0f, 13.0f, -7.5f), shade(pal::woodDark, 1.1f));
        g.fillPolygon(L.rect(-13.0f, 7.5f, 13.0f, 11.0f), shade(pal::woodDark, 1.1f));
        SDL_FPoint ax0 = L.p(-3.0f, -11.0f);
        SDL_FPoint ax1 = L.p(-3.0f, 11.0f);
        g.line(ax0.x, ax0.y, ax1.x, ax1.y, 2.6f * s, pal::iron);
        float bx = loaded ? -17.0f : -4.0f + 19.0f * recoil;
        SDL_FPoint a0 = L.p(-3.0f, 0.0f);
        SDL_FPoint a1 = L.p(bx, 0.0f);
        g.line(a0.x, a0.y, a1.x, a1.y, 4.0f * s, shade(pal::wood, 1.2f));
        g.fillCircle(a1.x, a1.y, 5.6f * s, pal::woodDark);
        if (loaded) {
            g.fillCircleGradient(a1.x - 0.8f * s, a1.y - 0.8f * s, 4.2f * s, pal::stoneLight, pal::stoneDark);
        }
    } else if (typeId == 2) {
        g.fillCircle(cx, cy, 9.0f * s, shade(pal::iron, 0.85f));
        g.fillPolygon(L.rect(-13.0f, -3.0f, 13.0f, 3.0f), pal::woodDark);
        g.fillPolygon(L.rect(-13.0f, -3.0f, 13.0f, -1.0f), shade(pal::wood, 1.15f));
        float stringX = loaded ? -9.0f : 3.0f + recoil * 2.0f;
        float bowR = 13.0f;
        float span = 1.15f;
        SDL_FPoint tipA = L.p(2.0f + std::cos(span) * bowR, std::sin(span) * bowR);
        SDL_FPoint tipB = L.p(2.0f + std::cos(span) * bowR, -std::sin(span) * bowR);
        SDL_FPoint nock = L.p(stringX, 0.0f);
        g.line(tipA.x, tipA.y, nock.x, nock.y, 0.9f * s, rgba(230, 222, 200, 220));
        g.line(tipB.x, tipB.y, nock.x, nock.y, 0.9f * s, rgba(230, 222, 200, 220));
        SDL_FPoint bc = L.p(2.0f, 0.0f);
        g.strokeArc(bc.x, bc.y, bowR * s, aim - span, aim + span, 3.4f * s, shade(pal::woodDark, 1.25f));
        if (loaded) {
            SDL_FPoint b0 = L.p(-9.0f, 0.0f);
            SDL_FPoint b1 = L.p(15.0f, 0.0f);
            g.line(b0.x, b0.y, b1.x, b1.y, 1.8f * s, pal::stoneLight);
            g.fillTriangle(L.p(15.0f, -2.6f), L.p(15.0f, 2.6f), L.p(20.0f, 0.0f), pal::stoneLight);
        }
        g.fillCircle(cx, cy, 3.0f * s, pal::iron);
    } else if (typeId == 3) {
        g.fillEllipse(L.p(-2.5f, 0.0f).x, L.p(-2.5f, 0.0f).y, 7.5f * s, 7.5f * s, rgba(54, 96, 72));
        g.fillCircle(L.p(-3.0f, 0.0f).x, L.p(-3.0f, 0.0f).y, 5.6f * s, rgba(70, 122, 90));
        SDL_FPoint head = L.p(0.5f, 0.0f);
        g.fillCircle(head.x, head.y, 4.2f * s, rgba(226, 186, 146));
        g.fillCircle(L.p(-0.6f, 0.0f).x, L.p(-0.6f, 0.0f).y, 3.6f * s, rgba(60, 100, 74));
        SDL_FPoint bc = L.p(3.0f, 0.0f);
        float span = 1.05f;
        g.strokeArc(bc.x, bc.y, 9.5f * s, aim - span, aim + span, 2.0f * s, shade(pal::wood, 1.35f));
        SDL_FPoint tA = L.p(3.0f + std::cos(span) * 9.5f, std::sin(span) * 9.5f);
        SDL_FPoint tB = L.p(3.0f + std::cos(span) * 9.5f, -std::sin(span) * 9.5f);
        SDL_FPoint nock = L.p(loaded ? 1.0f : 6.0f, 0.0f);
        g.line(tA.x, tA.y, nock.x, nock.y, 0.8f * s, rgba(240, 232, 210, 220));
        g.line(tB.x, tB.y, nock.x, nock.y, 0.8f * s, rgba(240, 232, 210, 220));
        if (loaded) {
            SDL_FPoint a0 = L.p(1.0f, 0.0f);
            SDL_FPoint a1 = L.p(14.0f, 0.0f);
            g.line(a0.x, a0.y, a1.x, a1.y, 1.2f * s, rgba(214, 196, 160));
        }
    }

    if (level > 1) {
        float total = (level - 1) * 9.0f * s;
        for (int i = 0; i < level; i++) {
            float px = cx - total * 0.5f + i * 9.0f * s;
            float py = cy - 20.0f * s;
            std::vector<SDL_FPoint> d = {{px, py - 3.6f * s}, {px + 3.6f * s, py}, {px, py + 3.6f * s}, {px - 3.6f * s, py}};
            std::vector<SDL_FPoint> o = {{px, py - 4.8f * s}, {px + 4.8f * s, py}, {px, py + 4.8f * s}, {px - 4.8f * s, py}};
            g.fillPolygon(o, rgba(40, 28, 10, 220));
            g.fillPolygon(d, pal::gold);
        }
    }
}

void Tower::draw(Gfx& g) const {
    if (selected) {
        float pulse = 0.5f + 0.5f * std::sin(g.time() * 5.0f);
        g.strokeCircle(x, y, 23.0f + pulse * 1.5f, 2.0f, withAlpha(pal::gold, 200));
    }
    bool loaded = cooldown <= 0.05f || recoil <= 0.0f;
    if (type.projectile == ProjectileKind::Boulder) loaded = cooldown <= (1.0f / fireRate()) * 0.45f && !ammo.empty();
    if (ammo.empty()) loaded = false;
    drawModel(g, type.typeId, x, y, 1.0f, aim, level, recoil, loaded);

    if (!ammo.full()) {
        float w = 30.0f;
        float bx = x - w * 0.5f;
        float by = y + 24.0f;
        g.fillRoundRect(bx - 1.0f, by - 1.0f, w + 2.0f, 5.0f, 2.5f, rgba(8, 12, 20, 200));
        Color c = ammo.empty() ? pal::danger : pal::supply;
        g.fillRoundRect(bx, by, std::max(1.5f, w * ammo.fraction()), 3.0f, 1.5f, c);
    }
    if (ammo.empty()) {
        float blink = 0.5f + 0.5f * std::sin(g.time() * 8.0f);
        g.fillCircle(x + 15.0f, y - 15.0f, 6.5f, withAlpha(pal::danger, static_cast<int>(150 + 100 * blink)));
        g.textCentered(FontId::SmallBold, "!", x + 15.0f, y - 15.5f, pal::uiText);
    }
}
