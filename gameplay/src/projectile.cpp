#include "projectile.h"
#include "enemy.h"
#include "game.h"
#include "gfx.h"
#include <algorithm>
#include <cmath>

namespace {
const float PI_F = 3.14159265358979f;
}

Projectile::Projectile(ProjectileKind k, float sx, float sy, int target, float tx, float ty, int dmg, float splashRadius,
                       float spd)
    : Entity(sx, sy, 1), kind(k), targetId(target), startX(sx), startY(sy), targetX(tx), targetY(ty), damage(dmg),
      splash(splashRadius), speed(spd), flightTime(0.0f), elapsed(0.0f), height(0.0f), dirX(1.0f), dirY(0.0f),
      trailTimer(0.0f) {
    float dx = tx - sx;
    float dy = ty - sy;
    float d = std::sqrt(dx * dx + dy * dy);
    if (d > 0.001f) {
        dirX = dx / d;
        dirY = dy / d;
    }
    flightTime = std::clamp(d / speed, 0.55f, 1.0f);
}

void Projectile::update(float dt, Game& game) {
    if (kind == ProjectileKind::Boulder) {
        elapsed += dt;
        float t = std::min(1.0f, elapsed / flightTime);
        x = startX + (targetX - startX) * t;
        y = startY + (targetY - startY) * t;
        float dist = std::sqrt((targetX - startX) * (targetX - startX) + (targetY - startY) * (targetY - startY));
        height = std::sin(t * PI_F) * (26.0f + dist * 0.18f);
        if (t >= 1.0f) impact(game);
        return;
    }

    Enemy* e = game.findEnemy(targetId);
    if (e && e->isAlive()) {
        targetX = e->getX();
        targetY = e->getY();
    }
    float dx = targetX - x;
    float dy = targetY - y;
    float d = std::sqrt(dx * dx + dy * dy);
    if (d > 0.001f) {
        dirX = dx / d;
        dirY = dy / d;
    }
    float step = speed * dt;
    float hitRadius = (e && e->isAlive()) ? e->getRadius() * 0.6f : 2.0f;
    if (d <= step + hitRadius) {
        x = targetX;
        y = targetY;
        impact(game);
        return;
    }
    x += dirX * step;
    y += dirY * step;

    if (kind == ProjectileKind::Cannonball) {
        trailTimer += dt;
        if (trailTimer > 0.03f) {
            trailTimer = 0.0f;
            game.fx().trail(x, y);
        }
    }
}

void Projectile::impact(Game& game) {
    if (!isAlive()) return;
    destroy();
    if (splash > 0.0f) {
        game.splashDamage(x, y, splash, damage);
        if (kind == ProjectileKind::Boulder) game.fx().explosion(x, y, splash, rgba(196, 170, 130));
        else game.fx().explosion(x, y, splash, rgba(255, 190, 110));
        return;
    }
    Enemy* e = game.findEnemy(targetId);
    if (e && e->isAlive()) {
        game.damageEnemy(*e, damage);
        game.fx().sparks(x, y, rgba(255, 236, 190));
    }
}

void Projectile::draw(Gfx& g) const {
    switch (kind) {
    case ProjectileKind::Cannonball:
        g.glow(x + 1.0f, y + 4.0f, 5.0f, 2.5f, rgba(0, 0, 0, 90));
        g.fillCircle(x, y, 4.4f, rgba(30, 32, 38));
        g.fillCircle(x - 1.2f, y - 1.2f, 1.6f, rgba(150, 154, 166));
        break;
    case ProjectileKind::Boulder: {
        float shadowR = 6.0f - height * 0.05f;
        g.glow(x, y + 3.0f, std::max(3.0f, shadowR * 1.4f), std::max(2.0f, shadowR * 0.7f), rgba(0, 0, 0, 110));
        float by = y - height;
        float r = 6.5f + height * 0.03f;
        g.fillCircleGradient(x - 1.0f, by - 1.0f, r, pal::stoneLight, pal::stoneDark);
        g.strokeArc(x, by, r * 0.55f, elapsed * 9.0f, elapsed * 9.0f + 1.6f, 1.2f, withAlpha(pal::stoneDark, 200));
        break;
    }
    case ProjectileKind::Bolt: {
        float tx = x - dirX * 16.0f;
        float ty = y - dirY * 16.0f;
        g.line(tx, ty, x, y, 2.4f, pal::woodDark);
        SDL_FPoint tip = {x + dirX * 5.0f, y + dirY * 5.0f};
        SDL_FPoint l = {x - dirY * 2.6f, y + dirX * 2.6f};
        SDL_FPoint r = {x + dirY * 2.6f, y - dirX * 2.6f};
        g.fillTriangle(l, r, tip, pal::stoneLight);
        break;
    }
    case ProjectileKind::Arrow: {
        float tx = x - dirX * 13.0f;
        float ty = y - dirY * 13.0f;
        g.line(tx, ty, x, y, 1.5f, rgba(206, 184, 140));
        SDL_FPoint tip = {x + dirX * 3.5f, y + dirY * 3.5f};
        SDL_FPoint l = {x - dirY * 1.8f, y + dirX * 1.8f};
        SDL_FPoint r = {x + dirY * 1.8f, y - dirX * 1.8f};
        g.fillTriangle(l, r, tip, pal::stoneLight);
        SDL_FPoint f0 = {tx + dirX * 3.0f, ty + dirY * 3.0f};
        g.fillTriangle({tx - dirY * 2.4f, ty + dirX * 2.4f}, {tx + dirY * 2.4f, ty - dirX * 2.4f}, f0, rgba(236, 236, 230));
        break;
    }
    }
}
