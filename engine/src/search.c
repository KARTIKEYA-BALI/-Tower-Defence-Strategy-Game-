#include <stddef.h>
#include "search.h"
#include "heap.h"

static int visit_order;

static int char_index(char c){
    if(c==' ') return 0;
    if(c>='0' && c<='9') return 1+(c-'0');
    if(c>='a' && c<='z') return 11+(c-'a');
    if(c>='A' && c<='Z') return 11+(c-'A');
    return -1;
}

static int clamp_score(int score){
    if(score<0) return 0;
    if(score>TRIE_MAX_SCORE) return TRIE_MAX_SCORE;
    return score;
}

static void clear_node(Trie *t,int n){
    for(int i=0;i<TRIE_ALPHABET;i++) t->nodes[n].child[i]=-1;
    t->nodes[n].is_word=0;
    t->nodes[n].tower_id=-1;
    t->nodes[n].score=0;
}

static int text_ok(const char *s,int allow_empty){
    int len=0;
    if(s==NULL) return 0;
    for(;s[len]!='\0';len++){
        if(len>=TRIE_MAX_NAME-1) return 0;
        if(char_index(s[len])<0) return 0;
    }
    if(len==0 && !allow_empty) return 0;
    return 1;
}

static int walk(const Trie *t,const char *s){
    int n=0;
    for(int i=0;s[i]!='\0';i++){
        n=t->nodes[n].child[char_index(s[i])];
        if(n==-1) return -1;
    }
    return n;
}

static int find_word(const Trie *t,const char *name){
    if(t==NULL || !t->ready || !text_ok(name,0)) return -1;
    int n=walk(t,name);
    if(n==-1 || !t->nodes[n].is_word) return -1;
    return n;
}

int trie_init(Trie *t){
    if(t==NULL) return 0;
    t->node_count=1;
    t->word_count=0;
    clear_node(t,0);
    t->ready=1;
    return 1;
}

void trie_shutdown(Trie *t){
    if(t==NULL) return;
    t->node_count=0;
    t->word_count=0;
    t->ready=0;
}

int trie_insert(Trie *t,const char *name,int id,int score){
    int n=0,i,needed=0;
    if(t==NULL || !t->ready || id<0 || !text_ok(name,0)) return 0;
    for(i=0;name[i]!='\0';i++){
        if(n!=-1) n=t->nodes[n].child[char_index(name[i])];
        if(n==-1) needed++;
    }
    if(n!=-1 && t->nodes[n].is_word) return 0;
    if(t->node_count+needed>TRIE_MAX_NODES) return 0;
    n=0;
    for(i=0;name[i]!='\0';i++){
        int c=char_index(name[i]);
        if(t->nodes[n].child[c]==-1){
            clear_node(t,t->node_count);
            t->nodes[n].child[c]=t->node_count;
            t->node_count++;
        }
        n=t->nodes[n].child[c];
    }
    t->nodes[n].is_word=1;
    t->nodes[n].tower_id=id;
    t->nodes[n].score=clamp_score(score);
    t->word_count++;
    return 1;
}

int trie_lookup(const Trie *t,const char *name,int *id,int *score){
    int n=find_word(t,name);
    if(n==-1) return 0;
    if(id) *id=t->nodes[n].tower_id;
    if(score) *score=t->nodes[n].score;
    return 1;
}

int trie_add_score(Trie *t,const char *name,int amount){
    int n=find_word(t,name);
    if(n==-1) return 0;
    t->nodes[n].score=clamp_score(t->nodes[n].score+amount);
    return 1;
}

static int collect(const Trie *t,int n,Heap *h){
    if(t->nodes[n].is_word){
        int priority=-t->nodes[n].score*TRIE_MAX_NODES+visit_order;
        visit_order++;
        if(!heap_push(h,priority,t->nodes[n].tower_id)) return 0;
    }
    for(int c=0;c<TRIE_ALPHABET;c++){
        if(t->nodes[n].child[c]!=-1 && !collect(t,t->nodes[n].child[c],h)) return 0;
    }
    return 1;
}

static int already_listed(const int out_ids[],int found,int id){
    for(int i=0;i<found;i++){
        if(out_ids[i]==id) return 1;
    }
    return 0;
}

int trie_search(const Trie *t,const char *prefix,int out_ids[],int k){
    static Heap h;
    HeapItem item;
    int found=0;
    if(t==NULL || !t->ready || out_ids==NULL || k<=0 || !text_ok(prefix,1)) return 0;
    int start=walk(t,prefix);
    if(start==-1) return 0;
    heap_init(&h);
    visit_order=0;
    if(!collect(t,start,&h)) return 0;
    while(found<k && heap_pop(&h,&item)){
        if(already_listed(out_ids,found,item.value)) continue;
        out_ids[found]=item.value;
        found++;
    }
    return found;
}

int trie_word_count(const Trie *t){
    return t ? t->word_count : 0;
}

int trie_node_count(const Trie *t){
    return t ? t->node_count : 0;
}
