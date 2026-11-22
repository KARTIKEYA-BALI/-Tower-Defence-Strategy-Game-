#ifndef RECORD_BOOK_H
#define RECORD_BOOK_H

#include <map>
#include <string>

struct MapRecord {
    int bestScore;
    int bestTimeMs;
    int plays;
    int wins;
};

struct RecordResult {
    bool newBestScore;
    bool newBestTime;
};

class RecordBook {
public:
    static constexpr int POINTS_PER_WAVE = 100;
    static constexpr int POINTS_PER_KILL = 10;
    static constexpr int WIN_BONUS = 500;
    static constexpr int POINTS_PER_HEALTH = 5;

    RecordBook();

    bool load(const std::string& path);
    bool save() const;
    void clear();

    MapRecord get(const std::string& mapKey) const;
    RecordResult submit(const std::string& mapKey, int score, bool won, int timeMs);

    static std::string mapKey(const std::string& mapName);
    static int scoreFor(int wavesCleared, int enemiesDefeated, int castleHealth, bool won);
    static std::string formatTime(int ms);

private:
    std::string file;
    std::map<std::string, MapRecord> records;
};

#endif
