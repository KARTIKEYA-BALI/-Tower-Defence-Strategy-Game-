#ifndef EFFECTS_H
#define EFFECTS_H

#include "gfx.h"
#include <string>
#include <vector>

class Effects {
public:
    void update(float dt);
    void draw(Gfx& g) const;
    void clear();

    void sparks(float x, float y, Color c);
    void smoke(float x, float y, int count);
    void trail(float x, float y);
    void flash(float x, float y, Color c);
    void explosion(float x, float y, float radius, Color c);
    void death(float x, float y, Color body, float size);
    void floatText(float x, float y, const std::string& text, Color c);
    void rubble(float x, float y);
    void streak(float x0, float y0, float x1, float y1, Color c);

private:
    struct Particle {
        float x, y, vx, vy;
        float life, maxLife;
        float size, grow;
        float drag, gravity;
        Color color;
        bool additive;
    };
    struct Ring {
        float x, y, r0, r1, life, maxLife;
        Color color;
    };
    struct FloatingText {
        float x, y, life, maxLife;
        std::string text;
        Color color;
        float ax, ay;
    };
    struct Spark {
        Particle p;
        float ax, ay;
    };
    struct Streak {
        float x0, y0, x1, y1, life, maxLife;
        Color color;
    };

    std::vector<Spark> particles;
    std::vector<Ring> rings;
    std::vector<FloatingText> texts;
    std::vector<Streak> streaks;

    static float rnd(float lo, float hi);
    void add(const Particle& p);
};

#endif
