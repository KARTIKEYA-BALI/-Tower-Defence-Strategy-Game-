#ifndef HUFFMAN_H
#define HUFFMAN_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
    int freq;
    int left;
    int right;
    int symbol;
} Node;


int huffman_compress(const uint8_t *in, size_t n, uint8_t **out, size_t *out_n);
int huffman_decompress(const uint8_t *in, size_t n, uint8_t **out, size_t *out_n);
void huffman_free(uint8_t *buf);

#endif
