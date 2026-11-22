#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <stdint.h>
#include "huffman.h"

static int passed = 0;
static int failed = 0;

static void check(int condition, const char *name) {
    if (condition) {
        printf("PASS: %s\n", name);
        passed++;
    } else {
        printf("FAIL: %s\n", name);
        failed++;
    }
}

static int roundtrip(const uint8_t *data, size_t n, size_t *compressed_n) {
    uint8_t *packed = NULL;
    uint8_t *unpacked = NULL;
    size_t unpacked_n = 0;
    int ok;
    *compressed_n = 0;
    if (!huffman_compress(data, n, &packed, compressed_n)) return 0;
    if (!huffman_decompress(packed, *compressed_n, &unpacked, &unpacked_n)) {
        huffman_free(packed);
        return 0;
    }
    ok = (unpacked_n == n) && (n == 0 || memcmp(unpacked, data, n) == 0);
    huffman_free(packed);
    huffman_free(unpacked);
    return ok;
}

static void check_size(const char *name, const uint8_t *data, size_t n, size_t expected) {
    size_t compressed_n = 0;
    char label[128];
    int ok = roundtrip(data, n, &compressed_n);
    sprintf(label, "%s: round trip gives back the same %u bytes", name, (unsigned)n);
    check(ok, label);
    sprintf(label, "%s: compressed size is %u bytes", name, (unsigned)expected);
    check(compressed_n == expected, label);
}

static int make_save(char *buf, int towers) {
    int len = sprintf(buf, "SIEGE1\ngold 120\nhealth 85\nwave 3\n");
    for (int i = 0; i < towers; i++) {
        len += sprintf(buf + len, "tower %d %d %d %d %d\n", i, i % 4, (3 * i) % 16, (5 * i) % 14, 1 + i % 3);
    }
    return len;
}

int main(void) {
    uint8_t *packed = NULL;
    uint8_t *unpacked = NULL;
    size_t packed_n = 0;
    size_t unpacked_n = 0;
    size_t size = 0;

    printf("--- Small inputs and edge cases\n");
    check_size("BCCABBDDAECCBBAEDDCC", (const uint8_t *)"BCCABBDDAECCBBAEDDCC", 20, 23);
    check_size("AAABC", (const uint8_t *)"AAABC", 5, 14);
    check_size("empty input", (const uint8_t *)"", 0, 7);
    check_size("one byte", (const uint8_t *)"Z", 1, 10);
    check_size("AAAAA (one kind of byte)", (const uint8_t *)"AAAAA", 5, 10);

    huffman_compress((const uint8_t *)"AAABC", 5, &packed, &packed_n);
    check(packed_n == 14 && packed[0] == 5 && packed[4] == 3 && packed[6] == 1 && packed[7] == 'A' && packed[8] == 3,
          "AAABC: header holds length 5, 3 kinds of byte, 1-byte counts, A x 3");
    check(packed[13] == 0xE2, "AAABC: packed bits are 11100010 (A=1, B=00, C=01, then padding)");
    huffman_free(packed);

    printf("--- Larger inputs\n");
    uint8_t all[256];
    for (int i = 0; i < 256; i++) all[i] = (uint8_t)i;
    check_size("all 256 byte values", all, 256, 775);

    uint8_t *big = malloc(70000);
    for (int i = 0; i < 70000; i++) big[i] = (uint8_t)(i % 7 == 0 ? 'B' : 'A');
    check_size("70000 bytes of A and B (2-byte counts)", big, 70000, 8763);

    srand(1);
    for (int i = 0; i < 70000; i++) big[i] = (uint8_t)rand();
    check_size("70000 random bytes (cannot shrink)", big, 70000, 70775);

    int deep_n = 0;
    long f1 = 1, f2 = 1;
    for (int s = 0; s < 17 && deep_n < 4000; s++) {
        long f = (s < 2) ? 1 : f1 + f2;
        if (s >= 2) { f1 = f2; f2 = f; }
        for (long j = 0; j < f && deep_n < 4000; j++) big[deep_n++] = (uint8_t)s;
    }
    check(roundtrip(big, (size_t)deep_n, &size), "very uneven counts (deep tree) round trip");

    srand(42);
    int bad = 0;
    for (int t = 0; t < 1000; t++) {
        size_t n = (size_t)(rand() % 3000);
        int kinds = 1 + rand() % 256;
        for (size_t i = 0; i < n; i++) big[i] = (uint8_t)(rand() % kinds);
        if (!roundtrip(big, n, &size)) bad++;
    }
    check(bad == 0, "1000 random inputs all round trip");

    printf("--- Game save files\n");
    char save[4096];
    int towers[3] = {3, 10, 30};
    size_t expected[3] = {97, 158, 317};
    size_t got[3];
    int lens[3];
    for (int c = 0; c < 3; c++) {
        char label[80];
        lens[c] = make_save(save, towers[c]);
        sprintf(label, "save with %d towers round trip", towers[c]);
        check(roundtrip((uint8_t *)save, (size_t)lens[c], &got[c]), label);
        sprintf(label, "save with %d towers compresses to %u bytes", towers[c], (unsigned)expected[c]);
        check(got[c] == expected[c], label);
    }
    check(got[1] < (size_t)lens[1] && got[2] < (size_t)lens[2], "saves with 10 and 30 towers get smaller");

    printf("--- Damaged files must be rejected, never crash\n");
    huffman_compress((const uint8_t *)"BCCABBDDAECCBBAEDDCC", 20, &packed, &packed_n);
    int cut_ok = 1;
    for (size_t cut = 0; cut < packed_n; cut++) {
        if (huffman_decompress(packed, cut, &unpacked, &unpacked_n)) {
            cut_ok = 0;
            huffman_free(unpacked);
        }
    }
    check(cut_ok, "every cut-short version of a file is rejected");

    uint8_t broken[64];
    memcpy(broken, packed, packed_n);
    broken[6] = 3;
    check(!huffman_decompress(broken, packed_n, &unpacked, &unpacked_n) && unpacked == NULL,
          "count width 3 is rejected and out stays NULL");
    memcpy(broken, packed, packed_n);
    broken[4] = 0x2C;
    broken[5] = 0x01;
    check(!huffman_decompress(broken, packed_n, &unpacked, &unpacked_n), "300 kinds of byte is rejected");
    memcpy(broken, packed, packed_n);
    broken[0] = 21;
    check(!huffman_decompress(broken, packed_n, &unpacked, &unpacked_n), "length that doesn't match the counts is rejected");
    memcpy(broken, packed, packed_n);
    broken[9] = 'A';
    check(!huffman_decompress(broken, packed_n, &unpacked, &unpacked_n), "the same byte twice in the table is rejected");
    memcpy(broken, packed, packed_n);
    broken[8] = 0;
    check(!huffman_decompress(broken, packed_n, &unpacked, &unpacked_n), "a count of 0 is rejected");
    check(!huffman_decompress(NULL, 10, &unpacked, &unpacked_n), "NULL input is rejected");
    huffman_free(packed);

    srand(9);
    for (int t = 0; t < 20000; t++) {
        uint8_t junk[80];
        size_t junk_n = (size_t)(rand() % 80);
        for (size_t i = 0; i < junk_n; i++) junk[i] = (uint8_t)rand();
        if (huffman_decompress(junk, junk_n, &unpacked, &unpacked_n)) huffman_free(unpacked);
    }
    check(1, "20000 random junk buffers did not crash");
    free(big);

    printf("\nCompression of game save files:\n");
    printf("  towers   original   compressed   size\n");
    for (int c = 0; c < 3; c++) {
        printf("  %6d   %8d   %10u   %3u%%\n", towers[c], lens[c], (unsigned)got[c],
               (unsigned)(100 * got[c] / (size_t)lens[c]));
    }

    printf("\n%d passed, %d failed\n", passed, failed);
    return failed == 0 ? 0 : 1;
}
