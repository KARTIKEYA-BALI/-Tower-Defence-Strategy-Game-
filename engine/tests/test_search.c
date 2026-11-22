#include <stdio.h>
#include <string.h>
#include "engine_core.h"
#include "search.h"

static Trie t;
static int passed = 0;
static int failed = 0;

static void check(int condition, const char *name)
{
    if (condition) {
        printf("PASS  %s\n", name);
        passed++;
    } else {
        printf("FAIL  %s\n", name);
        failed++;
    }
}

static int same(const int *got, int n, const int *want, int m)
{
    if (n != m) return 0;
    for (int i = 0; i < n; i++) {
        if (got[i] != want[i]) return 0;
    }
    return 1;
}

enum { CANNON, CATAPULT, CROSSBOW, ARCHER };

static int t_search(const char *prefix, int *out, int k)
{
    return trie_search(&t, prefix, out, k);
}

static void load_trie(void)
{
    trie_init(&t);
    trie_insert(&t, "cannon", CANNON, 8);
    trie_insert(&t, "catapult", CATAPULT, 10);
    trie_insert(&t, "crossbow", CROSSBOW, 6);
    trie_insert(&t, "archer", ARCHER, 4);
}

int main(void)
{
    int ids[16];
    int n, id, score;

    n = t_search("c", ids, 5);
    check(n == 0, "search before init finds nothing");
    check(trie_insert(&t, "cannon", 0, 8) == 0, "insert before init is refused");

    check(trie_init(&t) == 1, "init succeeds");
    check(t_search("", ids, 5) == 0, "empty shop finds nothing");
    check(trie_word_count(&t) == 0 && trie_node_count(&t) == 1, "empty shop has only the root");

    check(trie_insert(&t, "cannon", CANNON, 8) == 1, "insert cannon");
    check(trie_insert(&t, "catapult", CATAPULT, 10) == 1, "insert catapult");
    check(trie_insert(&t, "crossbow", CROSSBOW, 6) == 1, "insert crossbow");
    check(trie_insert(&t, "archer", ARCHER, 4) == 1, "insert archer");
    check(trie_word_count(&t) == 4, "four names stored");
    check(trie_node_count(&t) == 1 + 6 + 6 + 7 + 6, "shared prefix c and ca reuse nodes");

    check(trie_insert(&t, "cannon", 9, 1) == 0, "duplicate name refused");
    check(trie_insert(&t, "CANNON", 9, 1) == 0, "duplicate in capitals refused");
    check(trie_insert(&t, NULL, 9, 1) == 0, "NULL name refused");
    check(trie_insert(&t, "", 9, 1) == 0, "empty name refused");
    check(trie_insert(&t, "can-non", 9, 1) == 0, "name with a symbol refused");
    check(trie_insert(&t, "abcdefghijklmnopqrstuvwxyzabcdefgh", 9, 1) == 0, "name too long refused");
    check(trie_insert(&t, "ballista", -1, 1) == 0, "negative tower id refused");
    check(trie_word_count(&t) == 4, "refused names were not stored");

    check(trie_lookup(&t, "cannon", &id, &score) == 1 && id == CANNON && score == 8, "lookup cannon gives id and score");
    check(trie_lookup(&t, "cann", &id, &score) == 0, "a prefix is not a stored name");

    n = t_search("x", ids, 5);
    check(n == 0, "prefix matching no names");

    n = t_search("a", ids, 5);
    {
        int want[] = {ARCHER};
        check(same(ids, n, want, 1), "prefix matching one name");
    }

    n = t_search("c", ids, 5);
    {
        int want[] = {CATAPULT, CANNON, CROSSBOW};
        check(same(ids, n, want, 3), "prefix matching many names, best score first");
    }

    n = t_search("ca", ids, 5);
    {
        int want[] = {CATAPULT, CANNON};
        check(same(ids, n, want, 2), "prefix ca gives catapult then cannon");
    }

    n = t_search("CA", ids, 5);
    {
        int want[] = {CATAPULT, CANNON};
        check(same(ids, n, want, 2), "search ignores capitals");
    }

    n = t_search("cannon", ids, 5);
    check(n == 1 && ids[0] == CANNON, "full name finds itself");
    check(t_search("cannons", ids, 5) == 0, "longer than any name finds nothing");

    n = t_search("", ids, 5);
    {
        int want[] = {CATAPULT, CANNON, CROSSBOW, ARCHER};
        check(same(ids, n, want, 4), "empty prefix lists every tower by score");
    }

    n = t_search("c", ids, 2);
    {
        int want[] = {CATAPULT, CANNON};
        check(same(ids, n, want, 2), "k limits the results to the best k");
    }

    check(t_search("c", ids, 0) == 0, "k of zero gives nothing");
    check(trie_search(&t, "c", NULL, 5) == 0, "NULL output array is refused");
    check(trie_search(&t, NULL, ids, 5) == 0, "NULL prefix is refused");
    check(t_search("c!", ids, 5) == 0, "prefix with a symbol finds nothing");

    check(trie_add_score(&t, "crossbow", SHOP_PURCHASE_BONUS) == 1, "record a crossbow purchase");
    trie_add_score(&t, "crossbow", SHOP_PURCHASE_BONUS);
    trie_add_score(&t, "crossbow", SHOP_PURCHASE_BONUS);
    check(trie_lookup(&t, "crossbow", NULL, &score) == 1 && score == 6 + 3 * SHOP_PURCHASE_BONUS, "each purchase raises the score");
    n = t_search("c", ids, 5);
    {
        int want[] = {CROSSBOW, CATAPULT, CANNON};
        check(same(ids, n, want, 3), "bought tower moves to the top");
    }
    check(trie_add_score(&t, "tower", 1) == 0, "purchase of an unknown name is refused");
    check(trie_add_score(&t, "cros", 1) == 0, "purchase of a prefix is refused");

    check(trie_insert(&t, "ballista", 4, 4) == 1, "insert ballista with the same score as archer");
    n = t_search("", ids, 8);
    check(n == 5 && ids[3] == ARCHER && ids[4] == 4, "equal scores come out in alphabetical order");

    check(trie_insert(&t, "Siege Ram", 5, 1) == 1, "names may hold capitals and a space");
    n = t_search("siege r", ids, 5);
    check(n == 1 && ids[0] == 5, "prefix with a space works");

    trie_init(&t);
    for (int i = 0; i < 100; i++) {
        char name[8];
        snprintf(name, sizeof(name), "t%03d", i);
        trie_insert(&t, name, i, i);
    }
    check(trie_word_count(&t) == 100, "one hundred names stored");
    n = t_search("t", ids, 5);
    {
        int want[] = {99, 98, 97, 96, 95};
        check(same(ids, n, want, 5), "heap returns the top five of one hundred");
    }
    n = t_search("t05", ids, 3);
    {
        int want[] = {59, 58, 57};
        check(same(ids, n, want, 3), "deeper prefix narrows to its own subtree");
    }

    trie_shutdown(&t);
    check(t_search("t", ids, 5) == 0, "search after shutdown finds nothing");

    load_trie();
    n = t_search("c", ids, 5);
    {
        int want[] = {CATAPULT, CANNON, CROSSBOW};
        check(same(ids, n, want, 3), "init again forgets old purchases");
    }

    trie_init(&t);
    {
        char name[TRIE_MAX_NAME];
        int stored = 0;
        int i = 0;
        memset(name, 'a', TRIE_MAX_NAME - 1);
        name[TRIE_MAX_NAME - 1] = '\0';
        while (i < 100) {
            name[0] = (char)('a' + i % 26);
            name[1] = (char)('a' + i / 26);
            if (!trie_insert(&t, name, i, 1)) break;
            stored++;
            i++;
        }
        check(stored > 0 && stored < 100, "a full node pool refuses new names");
        check(trie_node_count(&t) <= TRIE_MAX_NODES, "node pool never overflows");
        check(trie_word_count(&t) == stored, "a refused name leaves nothing behind");
        n = t_search("a", ids, 16);
        check(n > 0, "search still works when the pool is full");
    }
    trie_shutdown(&t);


    {
        Trie other;
        trie_init(&other);
        trie_insert(&other, "cannon", 7, 1);
        load_trie();
        n = trie_search(&other, "c", ids, 5);
        check(n == 1 && ids[0] == 7, "two tries are kept apart");
        check(trie_search(NULL, "c", ids, 5) == 0 && trie_insert(NULL, "x", 1, 1) == 0, "NULL trie is refused");
    }

    trie_init(&t);
    trie_insert(&t, "snowy valley", 1, 5);
    trie_insert(&t, "valley", 1, 5);
    trie_insert(&t, "volcanic ridge", 4, 2);
    trie_insert(&t, "ridge", 4, 2);
    trie_insert(&t, "red rock", 6, 1);
    trie_insert(&t, "rock", 6, 1);
    trie_insert(&t, "rockfall", 6, 1);
    n = t_search("v", ids, 5);
    {
        int want[] = {1, 4};
        check(same(ids, n, want, 2), "two words of one name give the id once");
    }
    n = t_search("r", ids, 5);
    {
        int want[] = {4, 6};
        check(same(ids, n, want, 2), "duplicates are skipped and the order stays by score");
    }
    n = t_search("r", ids, 1);
    check(n == 1 && ids[0] == 4, "k counts distinct ids");

    check(shop_init() == 1, "shop: init");
    check(shop_insert("cannon", CANNON, 8) && shop_insert("catapult", CATAPULT, 10), "shop: insert two towers");
    n = shop_search("ca", ids, 5);
    {
        int want[] = {CATAPULT, CANNON};
        check(same(ids, n, want, 2), "shop: search goes through engine_core");
    }
    check(shop_record_purchase("cannon") == 1 && shop_record_purchase("cannon") == 1, "shop: record purchases");
    n = shop_search("ca", ids, 5);
    check(n == 2 && ids[0] == CANNON, "shop: purchases change the order");
    check(shop_search(NULL, ids, 5) == 0 && shop_search("c", NULL, 5) == 0, "shop: NULL arguments refused");
    shop_shutdown();
    check(shop_search("c", ids, 5) == 0, "shop: nothing after shutdown");

    n = map_search("", ids, 16);
    check(n == engine_level_count(), "maps: empty search lists every map");
    {
        int in_order = 1;
        for (int i = 0; i < n; i++) {
            if (ids[i] != i) in_order = 0;
        }
        check(in_order, "maps: empty search keeps the map order");
    }
    n = map_search("valley", ids, 16);
    check(n == 1 && strcmp(engine_level_name(ids[0]), "Snowy Valley") == 0, "maps: second word of a name is found");
    n = map_search("SNO", ids, 16);
    check(n == 1 && strcmp(engine_level_name(ids[0]), "Snowy Valley") == 0, "maps: first word, any case");
    n = map_search("v", ids, 16);
    check(n == 2, "maps: v finds Snowy Valley and Volcanic Ridge once each");
    check(map_search("zzz", ids, 16) == 0, "maps: no match");
    check(map_search(NULL, ids, 16) == 0 && map_search("a", NULL, 16) == 0, "maps: NULL arguments refused");

    printf("\n%d passed, %d failed\n", passed, failed);
    return failed == 0 ? 0 : 1;
}
