#include "wave_manager.h"
#include "game.h"
#include <algorithm>

WaveManager::WaveManager()
    : current(1), active(false), finished(false), groupIndex(0), spawnedInGroup(0), spawnTimer(0.0f), autoTimer(-1.0f) {
    waves.push_back({{{EnemyKind::Grunt, 8, 1.0f}}, 1.0f, "The first raiders"});
    waves.push_back({{{EnemyKind::Grunt, 8, 0.9f}, {EnemyKind::Runner, 6, 0.55f}}, 1.05f, "Runners on the flank"});
    waves.push_back({{{EnemyKind::Grunt, 10, 0.8f}, {EnemyKind::Brute, 3, 1.6f}, {EnemyKind::Runner, 6, 0.5f}}, 1.15f,
                     "Brutes at the gate"});
    waves.push_back({{{EnemyKind::Runner, 10, 0.45f}, {EnemyKind::Grunt, 12, 0.7f}, {EnemyKind::Brute, 5, 1.4f}}, 1.3f,
                     "The horde grows"});
    waves.push_back({{{EnemyKind::Grunt, 12, 0.65f},
                      {EnemyKind::Brute, 6, 1.3f},
                      {EnemyKind::Runner, 10, 0.4f},
                      {EnemyKind::Warlord, 1, 1.0f}},
                     1.45f,
                     "The Warlord comes"});
    reset();
}

void WaveManager::reset() {
    restartAt(1);
}

void WaveManager::restartAt(int waveNumber) {
    current = std::clamp(waveNumber, 1, totalWaves());
    active = false;
    finished = false;
    groupIndex = 0;
    spawnedInGroup = 0;
    spawnTimer = 0.0f;
    autoTimer = -1.0f;
}

bool WaveManager::startNextWave() {
    if (!readyToStart()) return false;
    active = true;
    groupIndex = 0;
    spawnedInGroup = 0;
    spawnTimer = 0.4f;
    autoTimer = -1.0f;
    return true;
}

int WaveManager::remainingToSpawn() const {
    if (!active) return 0;
    const Wave& w = waves[current - 1];
    int left = 0;
    for (int i = groupIndex; i < static_cast<int>(w.groups.size()); i++) {
        left += w.groups[i].count;
        if (i == groupIndex) left -= spawnedInGroup;
    }
    return std::max(0, left);
}

int WaveManager::earlyCallBonus() const {
    if (active || finished || autoTimer <= 0.0f) return 0;
    return static_cast<int>(autoTimer);
}

int WaveManager::clearBonus() const {
    return 20 + current * 5;
}

std::string WaveManager::waveTitle(int number) const {
    if (number < 1 || number > totalWaves()) return "";
    return waves[number - 1].title;
}

void WaveManager::update(float dt, Game& game, int enemiesAlive) {
    if (!active) {
        if (!finished && autoTimer > 0.0f) {
            autoTimer -= dt;
            if (autoTimer <= 0.0f) {
                autoTimer = -1.0f;
                game.requestWaveStart(false);
            }
        }
        return;
    }

    const Wave& w = waves[current - 1];
    if (groupIndex < static_cast<int>(w.groups.size())) {
        spawnTimer -= dt;
        if (spawnTimer <= 0.0f) {
            const Group& g = w.groups[groupIndex];
            game.spawnEnemy(g.kind, w.healthScale);
            spawnedInGroup++;
            spawnTimer = g.interval;
            if (spawnedInGroup >= g.count) {
                groupIndex++;
                spawnedInGroup = 0;
                spawnTimer = 1.6f;
            }
        }
        return;
    }

    if (enemiesAlive == 0) {
        active = false;
        game.onWaveCleared(current, clearBonus());
        if (current >= totalWaves()) {
            finished = true;
        } else {
            current++;
            autoTimer = 20.0f;
        }
    }
}
