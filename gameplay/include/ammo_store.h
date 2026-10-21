#ifndef AMMO_STORE_H
#define AMMO_STORE_H

class AmmoStore {
public:
    explicit AmmoStore(int maxAmmo);

    bool use();
    void refill(int amount);
    void refillFull();
    void setMax(int maxAmmo);

    int current() const { return count; }
    int max() const { return capacity; }
    bool empty() const { return count == 0; }
    bool full() const { return count == capacity; }
    float fraction() const;

private:
    int count;
    int capacity;
};

#endif
