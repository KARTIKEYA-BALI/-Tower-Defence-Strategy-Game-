#include "save_manager.h"
#include "engine_core.h"
#include <fstream>
#include <iterator>
#include <sstream>

std::string SaveManager::toText(const SaveData& data) {
    std::ostringstream out;
    out << "SIEGE1\n";
    out << "gold " << data.gold << "\n";
    out << "health " << data.health << "\n";
    out << "wave " << data.wave << "\n";
    for (const SavedTower& t : data.towers) {
        out << "tower " << t.objectId << " " << t.typeId << " " << t.cellX << " " << t.cellY << " " << t.level << "\n";
    }
    return out.str();
}

bool SaveManager::fromText(const std::string& text, SaveData& out) {
    std::istringstream in(text);
    std::string line;
    if (!std::getline(in, line)) return false;
    if (!line.empty() && line.back() == '\r') line.pop_back();
    if (line != "SIEGE1") return false;
    SaveData d = {0, 0, 1, {}};
    bool haveGold = false;
    bool haveHealth = false;
    bool haveWave = false;
    while (std::getline(in, line)) {
        std::istringstream ls(line);
        std::string key;
        if (!(ls >> key)) continue;
        if (key == "gold") haveGold = static_cast<bool>(ls >> d.gold);
        else if (key == "health") haveHealth = static_cast<bool>(ls >> d.health);
        else if (key == "wave") haveWave = static_cast<bool>(ls >> d.wave);
        else if (key == "tower") {
            SavedTower t = {0, 0, 0, 0, 1};
            if (ls >> t.objectId >> t.typeId >> t.cellX >> t.cellY >> t.level) d.towers.push_back(t);
        }
    }
    if (!haveGold || !haveHealth || !haveWave) return false;
    out = d;
    return true;
}

bool SaveManager::save(const SaveData& data, const std::string& path, std::size_t& rawBytes, std::size_t& storedBytes,
                       std::string& error) {
    std::string text = toText(data);
    uint8_t* packed = nullptr;
    std::size_t packedSize = 0;
    if (!save_compress(reinterpret_cast<const uint8_t*>(text.data()), text.size(), &packed, &packedSize) || !packed) {
        error = "Compression failed";
        return false;
    }
    std::ofstream f(path, std::ios::binary | std::ios::trunc);
    bool ok = static_cast<bool>(f);
    if (ok) {
        f.write(reinterpret_cast<const char*>(packed), static_cast<std::streamsize>(packedSize));
        ok = static_cast<bool>(f);
    }
    save_buffer_free(packed);
    if (!ok) {
        error = "Could not write " + path;
        return false;
    }
    rawBytes = text.size();
    storedBytes = packedSize;
    return true;
}

bool SaveManager::load(const std::string& path, SaveData& out, std::string& error) {
    std::ifstream f(path, std::ios::binary);
    if (!f) {
        error = "No saved game found";
        return false;
    }
    std::string bytes((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    uint8_t* plain = nullptr;
    std::size_t plainSize = 0;
    if (!save_decompress(reinterpret_cast<const uint8_t*>(bytes.data()), bytes.size(), &plain, &plainSize) || !plain) {
        error = "Save file is damaged";
        return false;
    }
    std::string text(reinterpret_cast<const char*>(plain), plainSize);
    save_buffer_free(plain);
    if (!fromText(text, out)) {
        error = "Save file is damaged";
        return false;
    }
    return true;
}

bool SaveManager::exists(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    return static_cast<bool>(f);
}
