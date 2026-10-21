#ifndef ENTITY_H
#define ENTITY_H

class Game;
class Gfx;

class Entity {
public:
    Entity(float x, float y, int maxHealth);
    virtual ~Entity() = default;

    virtual void update(float dt, Game& game) = 0;
    virtual void draw(Gfx& g) const = 0;
    virtual int layer() const = 0;

    int getId() const { return id; }
    float getX() const { return x; }
    float getY() const { return y; }
    bool isAlive() const { return alive; }
    int getHealth() const { return health; }
    int getMaxHealth() const { return maxHealth; }
    float healthFraction() const;

    bool takeDamage(int amount);
    void destroy();

protected:
    float x;
    float y;
    virtual void onDamaged(int amount);

private:
    int id;
    int health;
    int maxHealth;
    bool alive;
    static int nextId;
};

#endif
