#ifndef PROFILE_MANAGER_H
#define PROFILE_MANAGER_H

#include <cstddef>
#include <string>
#include <vector>

struct Profile {
    int id;
    std::string name;
};

enum class ProfileResult { Ok, EmptyName, NameTaken, Full, WriteFailed };

class ProfileManager {
public:
    static constexpr int MAX_PROFILES = 6;
    static constexpr std::size_t MAX_NAME = 14;

    explicit ProfileManager(const std::string& folder = "");

    void load();
    const std::vector<Profile>& all() const { return profiles; }
    const Profile* find(int id) const;
    int lastUsed() const { return last; }
    void setLastUsed(int id);

    ProfileResult create(const std::string& rawName, int& newId);
    bool remove(int id);

    std::string savePath(int id) const;
    std::string listPath() const { return folder + "profiles.txt"; }

    static bool validChar(char c);
    static std::string cleanName(const std::string& raw);

private:
    std::string folder;
    std::vector<Profile> profiles;
    int nextId;
    int last;

    bool nameTaken(const std::string& name) const;
    bool write() const;
};

#endif
