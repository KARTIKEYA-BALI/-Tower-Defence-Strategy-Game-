#ifndef SAVE_MANAGER_H
#define SAVE_MANAGER_H

#include <cstddef>
#include <string>
#include <vector>

struct SavedTower {
    int objectId;
    int typeId;
    int cellX;
    int cellY;
    int level;
};

struct SaveData {
    int gold;
    int health;
    int wave;
    std::vector<SavedTower> towers;
};

class SaveManager {
public:
    static std::string toText(const SaveData& data);
    static bool fromText(const std::string& text, SaveData& out);

    static bool save(const SaveData& data, const std::string& path, std::size_t& rawBytes, std::size_t& storedBytes,
                     std::string& error);
    static bool load(const std::string& path, SaveData& out, std::string& error);
    static bool exists(const std::string& path);
};

#endif
