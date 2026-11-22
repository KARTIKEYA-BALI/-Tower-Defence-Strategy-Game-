#include <cstdio>
#include <fstream>
#include <string>
#include "profile_manager.h"
#include "save_manager.h"

static int passed = 0;
static int failed = 0;

static void check(bool condition, const std::string& name) {
    if (condition) {
        std::printf("PASS: %s\n", name.c_str());
        passed++;
    } else {
        std::printf("FAIL: %s\n", name.c_str());
        failed++;
    }
}

static bool fileExists(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    return static_cast<bool>(f);
}

static void writeFile(const std::string& path, const std::string& text) {
    std::ofstream f(path, std::ios::binary | std::ios::trunc);
    f << text;
}

static std::string dir;

static void wipe() {
    std::remove((dir + "profiles.txt").c_str());
    for (int i = 0; i < 40; i++) std::remove((dir + "save_" + std::to_string(i) + ".dat").c_str());
}

static void testCleanName() {
    check(ProfileManager::cleanName("Kartikeya") == "Kartikeya", "clean: plain name kept");
    check(ProfileManager::cleanName("  Isha  ") == "Isha", "clean: spaces trimmed");
    check(ProfileManager::cleanName("Pari   Rawat") == "Pari Rawat", "clean: inner spaces collapsed");
    check(ProfileManager::cleanName("a/b\\c:d*e?") == "abcde", "clean: path characters removed");
    check(ProfileManager::cleanName("x_y-z 9") == "x_y-z 9", "clean: _ - digits allowed");
    check(ProfileManager::cleanName("abcdefghijklmnopqrstuvwxyz") == "abcdefghijklmn", "clean: cut to 14 characters");
    check(ProfileManager::cleanName("") == "", "clean: empty stays empty");
    check(ProfileManager::cleanName("   ") == "", "clean: only spaces becomes empty");
    check(ProfileManager::cleanName("\xC3\xA9t\xC3\xA9") == "t", "clean: non-ASCII bytes removed");
    check(ProfileManager::cleanName("abcdefghijklm  z") == "abcdefghijklm", "clean: no trailing space after cut");
    check(ProfileManager::validChar('A') && ProfileManager::validChar('7') && ProfileManager::validChar(' '),
          "validChar: letters digits space");
    check(!ProfileManager::validChar('/') && !ProfileManager::validChar('\n') && !ProfileManager::validChar('.'),
          "validChar: slash newline dot refused");
}

static void testCreateAndLimits() {
    wipe();
    ProfileManager pm(dir);
    pm.load();
    check(pm.all().empty(), "fresh: no profiles");
    check(pm.lastUsed() == -1, "fresh: no last profile");
    check(!fileExists(dir + "profiles.txt"), "fresh: load does not create a file");

    int id = -1;
    check(pm.create("Kartikeya", id) == ProfileResult::Ok && id == 0, "create: first id is 0");
    check(fileExists(dir + "profiles.txt"), "create: list file written");
    check(pm.create("  ", id) == ProfileResult::EmptyName, "create: empty name refused");
    check(pm.create("kartikeya", id) == ProfileResult::NameTaken, "create: same name any case refused");
    check(pm.create(" Kartikeya ", id) == ProfileResult::NameTaken, "create: same name with spaces refused");
    check(pm.create("Isha", id) == ProfileResult::Ok && id == 1, "create: second id is 1");
    check(pm.all().size() == 2, "create: two profiles");
    check(pm.find(1) && pm.find(1)->name == "Isha", "find: by id");
    check(pm.find(7) == nullptr, "find: unknown id gives null");

    pm.create("C", id);
    pm.create("D", id);
    pm.create("E", id);
    check(pm.create("F", id) == ProfileResult::Ok, "create: sixth allowed");
    check(pm.create("G", id) == ProfileResult::Full, "create: seventh refused");
    check(static_cast<int>(pm.all().size()) == ProfileManager::MAX_PROFILES, "create: list stops at 6");
}

static void testPersistence() {
    wipe();
    {
        ProfileManager pm(dir);
        pm.load();
        int id = -1;
        pm.create("Kartikeya", id);
        pm.create("Isha Rawat", id);
        pm.setLastUsed(1);
    }
    ProfileManager again(dir);
    again.load();
    check(again.all().size() == 2, "reload: both profiles back");
    check(again.all().size() == 2 && again.all()[1].name == "Isha Rawat", "reload: name with space kept");
    check(again.lastUsed() == 1, "reload: last used remembered");
    int id = -1;
    check(again.create("Sumedha", id) == ProfileResult::Ok && id == 2, "reload: next id continues");
    check(again.savePath(2) == dir + "save_2.dat", "savePath: per profile file");
}

static void testRemove() {
    wipe();
    ProfileManager pm(dir);
    pm.load();
    int a = -1;
    int b = -1;
    pm.create("Alpha", a);
    pm.create("Beta", b);
    pm.setLastUsed(a);

    SaveData d = {120, 80, 3, {{0, 1, 4, 6, 2, 9, 70}}, 0, -1, {}};
    std::size_t raw = 0;
    std::size_t stored = 0;
    std::string err;
    check(SaveManager::save(d, pm.savePath(a), raw, stored, err), "remove: save written for Alpha");
    check(SaveManager::save(d, pm.savePath(b), raw, stored, err), "remove: save written for Beta");

    check(SaveManager::save(d, pm.mapSavePath(a, 2), raw, stored, err), "remove: map save written for Alpha");
    check(pm.remove(a), "remove: existing profile");
    check(pm.mapSavePath(a, 2) == dir + "save_0_map2.dat", "mapSavePath: per profile and per map file");
    check(pm.mapSavePath(a, 2) != pm.mapSavePath(a, 3), "mapSavePath: each map has its own file");
    check(!fileExists(pm.savePath(a)), "remove: its save file deleted");
    check(!fileExists(pm.mapSavePath(a, 2)), "remove: its map saves deleted");
    check(fileExists(pm.savePath(b)), "remove: other save untouched");
    check(pm.lastUsed() == -1, "remove: last used cleared");
    check(!pm.remove(a), "remove: twice fails");
    check(!pm.remove(99), "remove: unknown id fails");

    int c = -1;
    pm.create("Gamma", c);
    check(c == 2, "remove: ids are never reused");
    check(!fileExists(pm.savePath(c)), "remove: new profile starts with no save");

    ProfileManager again(dir);
    again.load();
    check(again.all().size() == 2 && again.find(a) == nullptr, "remove: stays removed after reload");
}

static void testSavesAreSeparate() {
    wipe();
    ProfileManager pm(dir);
    pm.load();
    int a = -1;
    int b = -1;
    pm.create("One", a);
    pm.create("Two", b);
    SaveData da = {300, 90, 4, {{0, 0, 2, 2, 1, 5, 80}, {1, 3, 5, 5, 3, 12, 60}}, 2, -1, {}};
    SaveData db = {50, 10, 1, {}, 1, -1, {}};
    std::size_t raw = 0;
    std::size_t stored = 0;
    std::string err;
    SaveManager::save(da, pm.savePath(a), raw, stored, err);
    SaveManager::save(db, pm.savePath(b), raw, stored, err);
    SaveData ra;
    SaveData rb;
    check(SaveManager::load(pm.savePath(a), ra, err) && ra.gold == 300 && ra.wave == 4 && ra.towers.size() == 2,
          "separate: profile One reads its own save");
    check(SaveManager::load(pm.savePath(b), rb, err) && rb.gold == 50 && rb.wave == 1 && rb.towers.empty(),
          "separate: profile Two reads its own save");
    check(!SaveManager::exists(pm.savePath(99)), "separate: unknown profile has no save");
    check(ra.level == 2 && rb.level == 1, "save keeps which map was played");
    check(ra.towers.size() == 2 && ra.towers[1].ammo == 12 && ra.towers[1].health == 60 && ra.towers[0].ammo == 5,
          "save keeps each tower's ammo and health");
}

static void testOldSaveFormat() {
    SaveData d;
    check(SaveManager::fromText("SIEGE1\ngold 90\nhealth 70\nwave 2\ntower 3 1 4 5 2\n", d), "old save text still loads");
    check(d.level == 0, "old save: map defaults to the first one");
    check(d.towers.size() == 1 && d.towers[0].ammo == -1 && d.towers[0].health == -1 && d.towers[0].level == 2,
          "old save: missing ammo and health are marked unknown (-1)");
    check(SaveManager::fromText("SIEGE1\ngold 1\nhealth 1\nwave 1\nlevel x\ntower 0 0 1 1 1 4\n", d) && d.level == 0 &&
              d.towers.size() == 1 && d.towers[0].ammo == 4 && d.towers[0].health == -1,
          "bad level text ignored, ammo without health read");
    SaveData r;
    SaveData w = {10, 20, 3, {{1, 2, 3, 4, 1, 0, 1}}, 1, -1, {}};
    check(SaveManager::fromText(SaveManager::toText(w), r) && r.level == 1 && r.towers[0].ammo == 0 && r.towers[0].health == 1,
          "text round trip keeps zero ammo and 1 health");
}

static void testDamagedList() {
    wipe();
    writeFile(dir + "profiles.txt", "garbage\nprofile 0 Nope\n");
    ProfileManager pm(dir);
    pm.load();
    check(pm.all().empty(), "damaged: wrong header gives empty list");

    writeFile(dir + "profiles.txt",
              "PROFILES1\r\nnext 2\r\nlast 5\r\nprofile 0 Good\r\nprofile x Bad\r\nprofile -3 Neg\r\nprofile 0 Dup\r\n"
              "profile 4 good\r\nprofile 6\r\nprofile 9 Fine/Name\r\nlast oops\r\n");
    pm.load();
    check(pm.all().size() == 2, "damaged: only valid lines kept");
    check(pm.find(0) && pm.find(0)->name == "Good", "damaged: CRLF line read");
    check(pm.find(9) && pm.find(9)->name == "FineName", "damaged: bad characters stripped on load");
    check(pm.lastUsed() == -1, "damaged: last pointing nowhere ignored");
    int id = -1;
    pm.create("New", id);
    check(id == 10, "damaged: next id above every loaded id");

    std::string many = "PROFILES1\n";
    for (int i = 0; i < 9; i++) many += "profile " + std::to_string(i) + " P" + std::to_string(i) + "\n";
    writeFile(dir + "profiles.txt", many);
    pm.load();
    check(static_cast<int>(pm.all().size()) == ProfileManager::MAX_PROFILES, "damaged: too many lines capped at 6");
}

int main(int argc, char* argv[]) {
    dir = argc > 1 ? argv[1] : "";
    testCleanName();
    testCreateAndLimits();
    testPersistence();
    testRemove();
    testSavesAreSeparate();
    testOldSaveFormat();
    testDamagedList();
    wipe();
    std::printf("\n%d passed, %d failed\n", passed, failed);
    return failed == 0 ? 0 : 1;
}
