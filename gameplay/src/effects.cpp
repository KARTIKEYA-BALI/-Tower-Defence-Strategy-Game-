#include "effects.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace {
const float PI_F = 3.14159265358979f;
const size_t MAX_PARTICLES = 900;
}

float Effects::rnd(float lo, float hi) {
    return lo + (hi - lo) * (static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX));
}

void Effects::add(const Particle& p) {
    if (particles.size() < MAX_PARTICLES) particles.push_back(p);
}

void Effects::clear() {
    particles.clear();
    rings.clear();
    texts.clear();
}

void Effects::update(float dt) {
    for (Particle& p : particles) {
        p.life -= dt;
        float d = std::max(0.0f, 1.0f - p.drag * dt);
        p.vx *= d;
        p.vy *= d;
        p.vy += p.gravity * dt;
        p.x += p.vx * dt;
        p.y += p.vy * dt;
        p.size = std::max(0.0f, p.size + p.grow * dt);
    }
    particles.erase(std::remove_if(particles.begin(), particles.end(), [](const Particle& p) { return p.life <= 0.0f; }),
                    particles.end());
    for (Ring& r : rings) r.life -= dt;
    rings.erase(std::remove_if(rings.begin(), rings.end(), [](const Ring& r) { return r.life <= 0.0f; }), rings.end());
    for (FloatingText& t : texts) {
        t.life -= dt;
        t.y -= 26.0f * dt;
    }
    texts.erase(std::remove_if(texts.begin(), texts.end(), [](const FloatingText& t) { return t.life <= 0.0f; }),
                texts.end());
}

void Effects::draw(Gfx& g) const {
    for (const Ring& r : rings) {
        float t = 1.0f - r.life / r.maxLife;
        float radius = r.r0 + (r.r1 - r.r0) * (1.0f - (1.0f - t) * (1.0f - t));
        int a = static_cast<int>(r.color.a * (1.0f - t));
        g.glow(r.x, r.y, radius, radius * 0.8f, withAlpha(r.color, a / 3));
        g.strokeCircle(r.x, r.y, radius, 3.0f * (1.0f - t) + 1.0f, withAlpha(r.color, a));
    }
    for (const Particle& p : particles) {
        float t = std::clamp(p.life / p.maxLife, 0.0f, 1.0f);
        Color c = withAlpha(p.color, static_cast<int>(p.color.a * t));
        if (p.additive) g.glow(p.x, p.y, p.size * 2.2f, p.size * 2.2f, c);
        else g.fillCircle(p.x, p.y, std::max(0.4f, p.size), c);
    }
    for (const FloatingText& t : texts) {
        float k = std::clamp(t.life / t.maxLife, 0.0f, 1.0f);
        float a = k > 0.3f ? 1.0f : k / 0.3f;
        g.text(FontId::BodyBold, t.text, t.x, t.y, withAlpha(t.color, static_cast<int>(255 * a)), Align::Center, true);
    }
}

void Effects::sparks(float x, float y, Color c) {
    for (int i = 0; i < 6; i++) {
        float a = rnd(0.0f, 2.0f * PI_F);
        float s = rnd(60.0f, 150.0f);
        add({x, y, std::cos(a) * s, std::sin(a) * s, 0.25f, 0.25f, rnd(1.0f, 1.8f), -2.0f, 6.0f, 0.0f, c, false});
    }
    add({x, y, 0, 0, 0.12f, 0.12f, 6.0f, 20.0f, 0.0f, 0.0f, withAlpha(c, 180), true});
}

void Effects::smoke(float x, float y, int count) {
    for (int i = 0; i < count; i++) {
        float a = rnd(0.0f, 2.0f * PI_F);
        float s = rnd(8.0f, 28.0f);
        int v = static_cast<int>(rnd(150.0f, 200.0f));
        add({x + rnd(-3, 3), y + rnd(-3, 3), std::cos(a) * s, std::sin(a) * s - 14.0f, 0.7f, 0.7f, rnd(3.0f, 5.0f), 7.0f,
             2.0f, 0.0f, rgba(v, v, v, 120), false});
    }
}

void Effects::trail(float x, float y) {
    add({x, y, rnd(-6, 6), rnd(-10, -2), 0.35f, 0.35f, 2.2f, 5.0f, 2.0f, 0.0f, rgba(190, 190, 196, 90), false});
}

void Effects::flash(float x, float y, Color c) {
    add({x, y, 0, 0, 0.1f, 0.1f, 7.0f, 30.0f, 0.0f, 0.0f, withAlpha(c, 230), true});
}

void Effects::explosion(float x, float y, float radius, Color c) {
    rings.push_back({x, y, radius * 0.2f, radius, 0.35f, 0.35f, withAlpha(c, 200)});
    add({x, y, 0, 0, 0.16f, 0.16f, radius * 0.35f, radius * 0.6f, 0.0f, 0.0f, rgba(255, 220, 160, 200), true});
    for (int i = 0; i < 12; i++) {
        float a = rnd(0.0f, 2.0f * PI_F);
        float s = rnd(30.0f, radius * 2.4f);
        add({x, y, std::cos(a) * s, std::sin(a) * s, rnd(0.3f, 0.6f), 0.6f, rnd(1.5f, 3.0f), -2.0f, 5.0f, 0.0f,
             shade(c, rnd(0.6f, 1.0f)), false});
    }
    smoke(x, y, 5);
}

void Effects::death(float x, float y, Color body, float size) {
    for (int i = 0; i < 10; i++) {
        float a = rnd(0.0f, 2.0f * PI_F);
        float s = rnd(30.0f, 90.0f);
        add({x, y, std::cos(a) * s, std::sin(a) * s - 20.0f, rnd(0.35f, 0.6f), 0.6f, rnd(1.6f, size * 0.25f), -1.5f,
             4.0f, 160.0f, shade(body, rnd(0.7f, 1.1f)), false});
    }
    for (int i = 0; i < 6; i++) {
        float a = rnd(0.0f, 2.0f * PI_F);
        float s = rnd(10.0f, 35.0f);
        add({x, y, std::cos(a) * s, std::sin(a) * s - 18.0f, 0.9f, 0.9f, rnd(4.0f, 7.0f), 9.0f, 2.5f, 0.0f,
             rgba(70, 64, 72, 140), false});
    }
}

void Effects::floatText(float x, float y, const std::string& text, Color c) {
    texts.push_back({x, y, 1.0f, 1.0f, text, c});
}

void Effects::rubble(float x, float y) {
    for (int i = 0; i < 10; i++) {
        float a = rnd(-PI_F, 0.0f);
        float s = rnd(60.0f, 140.0f);
        add({x + rnd(-12, 12), y + rnd(-10, 6), std::cos(a) * s, std::sin(a) * s, rnd(0.4f, 0.7f), 0.7f, rnd(1.6f, 3.2f),
             -1.0f, 2.0f, 320.0f, shade({150, 150, 156, 255}, rnd(0.6f, 1.1f)), false});
    }
    rings.push_back({x, y, 8.0f, 46.0f, 0.4f, 0.4f, rgba(240, 80, 60, 200)});
}
