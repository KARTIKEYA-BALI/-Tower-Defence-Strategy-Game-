#ifndef WAVE_MANAGER_H
#define WAVE_MANAGER_H

#include "enemy.h"
#include <string>
#include <vector>

class Game;

class WaveManager {
public:
    WaveManager();

    void reset();
    void restartAt(int waveNumber);
    void update(float dt, Game& game, int enemiesAlive);

    bool startNextWave();
    int waveNumber() const { return current; }
    int totalWaves() const { return static_cast<int>(waves.size()); }
    bool inProgress() const { return active; }
    bool allWavesDone() const { return finished; }
    bool readyToStart() const { return !active && !finished; }
    float countdown() const { return autoTimer; }
    int remainingToSpawn() const;
    int earlyCallBonus() const;
    int clearBonus() const;
    std::string waveTitle(int number) const;

private:
    struct Group {
        EnemyKind kind;
        int count;
        float interval;
    };
    struct Wave {
        std::vector<Group> groups;
        float healthScale;
        std::string title;
    };

    std::vector<Wave> waves;
    int current;
    bool active;
    bool finished;
    int groupIndex;
    int spawnedInGroup;
    float spawnTimer;
    float autoTimer;
};

#endif
