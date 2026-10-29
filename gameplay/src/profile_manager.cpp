#include "profile_manager.h"
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <fstream>
#include <sstream>

namespace {

std::string lower(std::string s) {
    for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

}

ProfileManager::ProfileManager(const std::string& f) : folder(f), nextId(0), last(-1) {
}

void ProfileManager::load() {
    profiles.clear();
    nextId = 0;
    last = -1;
    std::ifstream in(listPath());
    if (!in) return;
    std::string line;
    if (!std::getline(in, line)) return;
    if (!line.empty() && line.back() == '\r') line.pop_back();
    if (line != "PROFILES1") return;
    int savedLast = -1;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        std::istringstream ls(line);
        std::string key;
        int value = 0;
        if (!(ls >> key)) continue;
        if (key == "next") {
            if (ls >> value) nextId = std::max(nextId, value);
        } else if (key == "last") {
            if (ls >> value) savedLast = value;
        } else if (key == "profile") {
            if (!(ls >> value) || value < 0) continue;
            std::string rest;
            std::getline(ls, rest);
            std::string name = cleanName(rest);
            if (name.empty() || find(value) || nameTaken(name) || static_cast<int>(profiles.size()) >= MAX_PROFILES) continue;
            profiles.push_back({value, name});
            nextId = std::max(nextId, value + 1);
        }
    }
    if (find(savedLast)) last = savedLast;
}

const Profile* ProfileManager::find(int id) const {
    for (const Profile& p : profiles) {
        if (p.id == id) return &p;
    }
    return nullptr;
}

void ProfileManager::setLastUsed(int id) {
    if (!find(id) || last == id) return;
    last = id;
    write();
}

ProfileResult ProfileManager::create(const std::string& rawName, int& newId) {
    std::string name = cleanName(rawName);
    if (name.empty()) return ProfileResult::EmptyName;
    if (static_cast<int>(profiles.size()) >= MAX_PROFILES) return ProfileResult::Full;
    if (nameTaken(name)) return ProfileResult::NameTaken;
    profiles.push_back({nextId, name});
    nextId++;
    if (!write()) {
        profiles.pop_back();
        nextId--;
        return ProfileResult::WriteFailed;
    }
    newId = profiles.back().id;
    return ProfileResult::Ok;
}

bool ProfileManager::remove(int id) {
    auto it = std::find_if(profiles.begin(), profiles.end(), [id](const Profile& p) { return p.id == id; });
    if (it == profiles.end()) return false;
    std::vector<Profile> before = profiles;
    int beforeLast = last;
    profiles.erase(it);
    if (last == id) last = -1;
    if (!write()) {
        profiles = before;
        last = beforeLast;
        return false;
    }
    std::remove(savePath(id).c_str());
    return true;
}

std::string ProfileManager::savePath(int id) const {
    return folder + "save_" + std::to_string(id) + ".dat";
}

bool ProfileManager::validChar(char c) {
    return std::isalnum(static_cast<unsigned char>(c)) || c == ' ' || c == '-' || c == '_';
}

std::string ProfileManager::cleanName(const std::string& raw) {
    std::string out;
    for (char c : raw) {
        if (!validChar(c)) continue;
        if (c == ' ' && (out.empty() || out.back() == ' ')) continue;
        if (out.size() >= MAX_NAME) break;
        out.push_back(c);
    }
    while (!out.empty() && out.back() == ' ') out.pop_back();
    return out;
}

bool ProfileManager::nameTaken(const std::string& name) const {
    std::string key = lower(name);
    for (const Profile& p : profiles) {
        if (lower(p.name) == key) return true;
    }
    return false;
}

bool ProfileManager::write() const {
    std::ofstream out(listPath(), std::ios::trunc);
    if (!out) return false;
    out << "PROFILES1\n";
    out << "next " << nextId << "\n";
    out << "last " << last << "\n";
    for (const Profile& p : profiles) out << "profile " << p.id << " " << p.name << "\n";
    out.flush();
    return static_cast<bool>(out);
}
