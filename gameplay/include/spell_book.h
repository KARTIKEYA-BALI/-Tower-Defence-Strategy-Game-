#ifndef SPELL_BOOK_H
#define SPELL_BOOK_H

#include "gfx.h"
#include <string>
#include <vector>

enum class SpellKind { Freeze, Mud, Fire };

constexpr int SPELL_KINDS = 3;
constexpr int SPELL_TIERS = 3;

struct SpellTier {
    int cost;
    float duration;
};

struct SpellType {
    SpellKind kind;
    std::string name;
    std::string title;
    int radius;
    float cooldown;
    SpellTier tiers[SPELL_TIERS];
    Color colour;
    std::string blurb;
    std::string effect;
};

class SpellBook {
public:
    static constexpr int SEARCH_ID_BASE = 100;
    static constexpr float MUD_SLOW = 0.45f;
    static constexpr float FIRE_TICK = 0.5f;
    static constexpr int FIRE_TICK_DAMAGE = 7;
    static constexpr float BOSS_FREEZE_SCALE = 0.5f;

    SpellBook();

    int count() const { return static_cast<int>(types.size()); }
    bool validKind(int kind) const { return kind >= 0 && kind < count(); }
    static bool validTier(int tier) { return tier >= 0 && tier < SPELL_TIERS; }
    const SpellType& type(int kind) const;
    std::vector<int> all() const;

    bool ready(int kind) const;
    float cooldownLeft(int kind) const;
    float cooldownFraction(int kind) const;
    void startCooldown(int kind);
    void update(float dt);
    void reset();

    static const char* tierName(int tier);
    static void drawIcon(Gfx& g, int kind, float cx, float cy, float r, float time);

private:
    std::vector<SpellType> types;
    float cooldowns[SPELL_KINDS];
};

#endif
