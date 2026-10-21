#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <limits.h>
#include "huffman.h"
#include "heap.h"

#define MAX_NODES 511
#define HEADER_FIXED 7



static Node nodes[MAX_NODES];
static int node_count;
static char codes[256][256];
static int code_len[256];

static int build_tree(const uint32_t freq[256]){
    static Heap h;
    HeapItem a,b;
    heap_init(&h);
    node_count=0;
    for(int s=0;s<256;s++){
        if(freq[s]==0) continue;
        nodes[node_count].freq=(int)freq[s];
        nodes[node_count].left=-1;
        nodes[node_count].right=-1;
        nodes[node_count].symbol=s;
        heap_push(&h,(int)freq[s],node_count);
        node_count++;
    }
    if(node_count==0) return -1;
    while(h.size>1){
        heap_pop(&h,&a);
        heap_pop(&h,&b);
        nodes[node_count].freq=a.priority+b.priority;
        nodes[node_count].left=a.value;
        nodes[node_count].right=b.value;
        nodes[node_count].symbol=-1;
        heap_push(&h,nodes[node_count].freq,node_count);
        node_count++;
    }
    heap_pop(&h,&a);
    return a.value;
}

static void make_codes(int node,char path[],int depth){
    if(nodes[node].left==-1){
        int s=nodes[node].symbol;
        if(depth==0){
            codes[s][0]=0;
            code_len[s]=1;
        }
        else{
            memcpy(codes[s],path,depth);
            code_len[s]=depth;
        }
        return;
    }
    path[depth]=0;
    make_codes(nodes[node].left,path,depth+1);
    path[depth]=1;
    make_codes(nodes[node].right,path,depth+1);
}

static void write_num(uint8_t *p,uint32_t value,int bytes){
    for(int i=0;i<bytes;i++) p[i]=(uint8_t)(value>>(8*i));
}

static uint32_t read_num(const uint8_t *p,int bytes){
    uint32_t value=0;
    for(int i=0;i<bytes;i++) value|=(uint32_t)p[i]<<(8*i);
    return value;
}

int huffman_compress(const uint8_t *in, size_t n, uint8_t **out, size_t *out_n){
    uint32_t freq[256]={0};
    uint32_t biggest=0;
    int k=0,w;
    size_t bits=0,size,pos,bitpos=0;
    uint8_t *buf;
    *out=NULL;
    *out_n=0;
    if(n>(size_t)INT_MAX) return 0;
    if(n>0 && in==NULL) return 0;
    for(size_t i=0;i<n;i++) freq[in[i]]++;
    for(int s=0;s<256;s++){
        if(freq[s]==0) continue;
        k++;
        if(freq[s]>biggest) biggest=freq[s];
    }
    w=(biggest<=255)?1:(biggest<=65535)?2:4;
    memset(code_len,0,sizeof(code_len));
    if(k>0){
        char path[256];
        int root=build_tree(freq);
        make_codes(root,path,0);
        for(int s=0;s<256;s++) bits+=(size_t)freq[s]*code_len[s];
    }
    size=HEADER_FIXED+(size_t)k*(1+w)+(bits+7)/8;
    buf=(uint8_t *)calloc(size,1);
    if(buf==NULL) return 0;
    write_num(buf,(uint32_t)n,4);
    write_num(buf+4,(uint32_t)k,2);
    buf[6]=(uint8_t)w;
    pos=HEADER_FIXED;
    for(int s=0;s<256;s++){
        if(freq[s]==0) continue;
        buf[pos]=(uint8_t)s;
        write_num(buf+pos+1,freq[s],w);
        pos+=1+w;
    }
    for(size_t i=0;i<n;i++){
        int s=in[i];
        for(int j=0;j<code_len[s];j++){
            if(codes[s][j]) buf[pos+bitpos/8]|=(uint8_t)(0x80>>(bitpos%8));
            bitpos++;
        }
    }
    *out=buf;
    *out_n=size;
    return 1;
}

int huffman_decompress(const uint8_t *in, size_t n, uint8_t **out, size_t *out_n){
    uint32_t freq[256]={0};
    uint32_t length;
    uint64_t total=0;
    int k,w;
    size_t pos,bitpos=0,nbits;
    uint8_t *buf;
    *out=NULL;
    *out_n=0;
    if(in==NULL || n<HEADER_FIXED) return 0;
    length=read_num(in,4);
    k=(int)read_num(in+4,2);
    w=in[6];
    if(k>256 || (w!=1 && w!=2 && w!=4)) return 0;
    if(length>(uint32_t)INT_MAX) return 0;
    if(n<HEADER_FIXED+(size_t)k*(1+w)) return 0;
    pos=HEADER_FIXED;
    for(int i=0;i<k;i++){
        int s=in[pos];
        uint32_t f=read_num(in+pos+1,w);
        if(f==0 || freq[s]!=0) return 0;
        freq[s]=f;
        total+=f;
        pos+=1+w;
    }
    if(total!=length) return 0;
    buf=(uint8_t *)malloc(length>0?length:1);
    if(buf==NULL) return 0;
    if(length>0){
        int root=build_tree(freq);
        nbits=(n-pos)*8;
        for(uint32_t i=0;i<length;i++){
            int c=root;
            if(nodes[c].left==-1){
                if(bitpos>=nbits){
                    free(buf);
                    return 0;
                }
                bitpos++;
            }
            while(nodes[c].left!=-1){
                if(bitpos>=nbits){
                    free(buf);
                    return 0;
                }
                int bit=(in[pos+bitpos/8]>>(7-bitpos%8))&1;
                bitpos++;
                c=bit?nodes[c].right:nodes[c].left;
            }
            buf[i]=(uint8_t)nodes[c].symbol;
        }
    }
    *out=buf;
    *out_n=length;
    return 1;
}

void huffman_free(uint8_t *buf){
    free(buf);
}
