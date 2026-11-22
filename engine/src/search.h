#ifndef SEARCH_H
#define SEARCH_H

#define TRIE_ALPHABET 37
#define TRIE_MAX_NODES 512
#define TRIE_MAX_NAME 32
#define TRIE_MAX_SCORE 1000000
#define SHOP_PURCHASE_BONUS 3

typedef struct {
    int child[TRIE_ALPHABET];
    int is_word;
    int tower_id;
    int score;
} TrieNode;

typedef struct {
    TrieNode nodes[TRIE_MAX_NODES];
    int node_count;
    int word_count;
    int ready;
} Trie;

int  trie_init(Trie *t);
void trie_shutdown(Trie *t);
int  trie_insert(Trie *t, const char *name, int id, int score);
int  trie_lookup(const Trie *t, const char *name, int *id, int *score);
int  trie_add_score(Trie *t, const char *name, int amount);
int  trie_search(const Trie *t, const char *prefix, int out_ids[], int k);
int  trie_word_count(const Trie *t);
int  trie_node_count(const Trie *t);

#endif
