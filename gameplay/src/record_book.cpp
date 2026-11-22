#include "record_book.h"
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <fstream>
#include <sstream>

RecordBook::RecordBook() {
}

void RecordBook::clear() {
    records.clear();
    file.clear();
}

bool RecordBook::load(const std::string& path) {
    records.clear();
    file = path;
    std::ifstream in(path);
    if (!in) return false;
    std::string line;
    if (!std::getline(in, line)) return false;
    if (!line.empty() && line.back() == '\r') line.pop_back();
    if (line != "RECORDS1") return false;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        std::istringstream ls(line);
        std::string tag;
        std::string key;
        MapRecord r = {-1, -1, 0, 0};
        if (!(ls >> tag >> key) || tag != "map") continue;
        if (!(ls >> r.bestScore >> r.bestTimeMs >> r.plays >> r.wins)) continue;
        if (r.plays < 0 || r.wins < 0 || r.wins > r.plays) continue;
        records[key] = r;
    }
    return true;
}

bool RecordBook::save() const {
    if (file.empty()) return false;
    std::ofstream out(file, std::ios::trunc);
    if (!out) return false;
    out << "RECORDS1\n";
    for (const auto& kv : records) {
        const MapRecord& r = kv.second;
        out << "map " << kv.first << " " << r.bestScore << " " << r.bestTimeMs << " " << r.plays << " " << r.wins << "\n";
    }
    out.flush();
    return static_cast<bool>(out);
}

MapRecord RecordBook::get(const std::string& mapKey) const {
    auto it = records.find(mapKey);
    if (it == records.end()) return {-1, -1, 0, 0};
    return it->second;
}

RecordResult RecordBook::submit(const std::string& mapKey, int score, bool won, int timeMs) {
    RecordResult result = {false, false};
    if (mapKey.empty()) return result;
    MapRecord r = get(mapKey);
    r.plays++;
    if (won) r.wins++;
    if (score > r.bestScore) {
        r.bestScore = score;
        result.newBestScore = true;
    }
    if (won && timeMs > 0 && (r.bestTimeMs < 0 || timeMs < r.bestTimeMs)) {
        r.bestTimeMs = timeMs;
        result.newBestTime = true;
    }
    records[mapKey] = r;
    save();
    return result;
}

std::string RecordBook::mapKey(const std::string& mapName) {
    std::string key;
    for (char c : mapName) {
        unsigned char u = static_cast<unsigned char>(c);
        if (std::isalnum(u)) key.push_back(static_cast<char>(std::tolower(u)));
        else if (!key.empty() && key.back() != '-') key.push_back('-');
    }
    while (!key.empty() && key.back() == '-') key.pop_back();
    return key;
}

int RecordBook::scoreFor(int wavesCleared, int enemiesDefeated, int castleHealth, bool won) {
    int score = POINTS_PER_WAVE * std::max(0, wavesCleared) + POINTS_PER_KILL * std::max(0, enemiesDefeated);
    if (won) score += WIN_BONUS + POINTS_PER_HEALTH * std::max(0, castleHealth);
    return score;
}

std::string RecordBook::formatTime(int ms) {
    if (ms < 0) return "--:--";
    int total = ms / 1000;
    char buf[16];
    std::snprintf(buf, sizeof(buf), "%d:%02d", total / 60, total % 60);
    return buf;
}
