#include "record_book.h"
#include "save_manager.h"
#include <cstdio>
#include <fstream>
#include <string>

static int passed = 0;
static int failed = 0;

static void check(bool condition, const char* name) {
    if (condition) {
        std::printf("PASS  %s\n", name);
        passed++;
    } else {
        std::printf("FAIL  %s\n", name);
        failed++;
    }
}

static const char* PATH = "test_records_tmp.txt";

static void testFormula() {
    check(RecordBook::scoreFor(0, 0, 100, false) == 0, "score: nothing done is 0");
    check(RecordBook::scoreFor(2, 15, 0, false) == 2 * 100 + 15 * 10, "score: waves and kills count on a loss");
    check(RecordBook::scoreFor(5, 111, 80, true) == 500 + 1110 + 500 + 400, "score: a win adds the bonus and the health");
    check(RecordBook::scoreFor(5, 111, 80, false) == 500 + 1110, "score: health only counts on a win");
    check(RecordBook::scoreFor(-3, -1, -5, true) == 500, "score: negative inputs are treated as 0");
}

static void testKeysAndTime() {
    check(RecordBook::mapKey("Snowy Valley") == "snowy-valley", "key: spaces become dashes, all lower case");
    check(RecordBook::mapKey("  Volcanic  Ridge ") == "volcanic-ridge", "key: extra spaces are dropped");
    check(RecordBook::mapKey("Greenfield") == "greenfield", "key: one word");
    check(RecordBook::formatTime(-1) == "--:--", "time: unknown shows dashes");
    check(RecordBook::formatTime(0) == "0:00", "time: zero");
    check(RecordBook::formatTime(65400) == "1:05", "time: minutes and seconds");
    check(RecordBook::formatTime(600000) == "10:00", "time: ten minutes");
}

static void testSubmit() {
    std::remove(PATH);
    RecordBook book;
    check(!book.load(PATH), "load: a missing file gives an empty book");
    MapRecord none = book.get("greenfield");
    check(none.bestScore == -1 && none.bestTimeMs == -1 && none.plays == 0 && none.wins == 0, "get: unknown map is empty");

    RecordResult r = book.submit("greenfield", 300, false, 90000);
    check(r.newBestScore && !r.newBestTime, "first loss: new best score, no time");
    MapRecord a = book.get("greenfield");
    check(a.bestScore == 300 && a.bestTimeMs == -1 && a.plays == 1 && a.wins == 0, "first loss is stored");

    r = book.submit("greenfield", 200, false, 50000);
    check(!r.newBestScore && !r.newBestTime && book.get("greenfield").bestScore == 300, "a lower score keeps the old best");

    r = book.submit("greenfield", 2100, true, 400000);
    check(r.newBestScore && r.newBestTime, "first win: best score and first time");
    r = book.submit("greenfield", 1900, true, 350000);
    check(!r.newBestScore && r.newBestTime, "a faster win with a lower score still sets the time");
    r = book.submit("greenfield", 2500, true, 380000);
    check(r.newBestScore && !r.newBestTime, "a slower win with a higher score sets only the score");
    MapRecord b = book.get("greenfield");
    check(b.bestScore == 2500 && b.bestTimeMs == 350000 && b.plays == 5 && b.wins == 3, "plays, wins and bests add up");

    r = book.submit("greenfield", 2600, true, -1);
    check(r.newBestScore && !r.newBestTime && book.get("greenfield").bestTimeMs == 350000, "an unknown time never counts");
    r = book.submit("", 9999, true, 1);
    check(!r.newBestScore && !r.newBestTime, "an empty map key is refused");

    book.submit("snowy-valley", 700, false, 1);
    RecordBook again;
    check(again.load(PATH), "load: the file written by submit reads back");
    MapRecord c = again.get("greenfield");
    check(c.bestScore == 2600 && c.bestTimeMs == 350000 && c.plays == 6 && c.wins == 4, "load: greenfield matches");
    check(again.get("snowy-valley").bestScore == 700 && again.get("snowy-valley").plays == 1, "load: second map matches");
}

static void testBadFile() {
    {
        std::ofstream out(PATH, std::ios::trunc);
        out << "RECORDS1\r\n";
        out << "map ok 10 -1 1 0\r\n";
        out << "map broken 5\n";
        out << "map cheat 9 9 1 5\n";
        out << "junk line\n";
        out << "map neg 9 9 -2 0\n";
    }
    RecordBook book;
    check(book.load(PATH), "bad lines: the file still loads");
    check(book.get("ok").bestScore == 10, "bad lines: good line kept (with Windows line endings)");
    check(book.get("broken").plays == 0 && book.get("cheat").plays == 0 && book.get("neg").plays == 0,
          "bad lines: short, impossible and negative lines are skipped");
    {
        std::ofstream out(PATH, std::ios::trunc);
        out << "NOTRECORDS\nmap ok 10 -1 1 0\n";
    }
    check(!book.load(PATH) && book.get("ok").plays == 0, "wrong header: nothing is loaded");
    std::remove(PATH);
}

static void testSaveTime() {
    SaveData d = {120, 80, 3, {}, 2, 245500, {}};
    SaveData out;
    check(SaveManager::fromText(SaveManager::toText(d), out) && out.timeMs == 245500, "save: play time round trip");
    SaveData unknown = {120, 80, 3, {}, 2, -1, {}};
    std::string text = SaveManager::toText(unknown);
    check(text.find("time") == std::string::npos, "save: unknown time is not written");
    check(SaveManager::fromText(text, out) && out.timeMs == -1, "save: old saves load with an unknown time");
    SaveData withTroops = {120, 80, 3, {}, 2, 1000, {{4, 5, 200}, {10, 2, 260}}};
    check(SaveManager::fromText(SaveManager::toText(withTroops), out) && out.troops.size() == 2 && out.troops[0].cellX == 4 &&
              out.troops[0].cellY == 5 && out.troops[0].health == 200 && out.troops[1].cellX == 10,
          "save: Cannon Carts round trip");
    check(SaveManager::fromText("SIEGE1\ngold 1\nhealth 1\nwave 1\ntroop 3\n", out) && out.troops.empty(),
          "save: a broken troop line is skipped");
}

int main() {
    testFormula();
    testKeysAndTime();
    testSubmit();
    testBadFile();
    testSaveTime();
    std::printf("\n%d passed, %d failed\n", passed, failed);
    return failed == 0 ? 0 : 1;
}
