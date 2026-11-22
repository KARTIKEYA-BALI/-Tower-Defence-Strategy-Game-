#include <stdio.h>
#include <stdlib.h>
#include "heap.h"

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

static int compare_ints(const void *a, const void *b)
{
    return *(const int *)a - *(const int *)b;
}

int main(void)
{
    Heap h;
    HeapItem item;
    int ok;

    heap_init(&h);
    check(heap_is_empty(&h), "new heap is empty");
    check(heap_pop(&h, &item) == 0, "pop on empty heap returns 0");

    int input[] = {5, 3, 8, 1, 9, 2};
    int expected[] = {1, 2, 3, 5, 8, 9};
    for (int k = 0; k < 6; k++) {
        heap_push(&h, input[k], input[k] * 10);
    }
    ok = 1;
    for (int k = 0; k < 6; k++) {
        heap_pop(&h, &item);
        if (item.priority != expected[k] || item.value != expected[k] * 10) {
            ok = 0;
        }
    }
    check(ok, "pops 1,2,3,5,8,9 in order with values attached");
    check(heap_is_empty(&h), "heap is empty after popping everything");

    heap_init(&h);
    int tricky[] = {1, 5, 3, 6, 7, 9};
    for (int k = 0; k < 6; k++) {
        heap_push(&h, tricky[k], k);
    }
    heap_pop(&h, &item);
    heap_pop(&h, &item);
    check(item.priority == 3, "picks the smaller child (3, not 5)");

    int numbers[1000];
    srand(42);
    heap_init(&h);
    for (int k = 0; k < 1000; k++) {
        numbers[k] = rand() % 500;
        heap_push(&h, numbers[k], k);
    }
    qsort(numbers, 1000, sizeof(int), compare_ints);
    ok = 1;
    for (int k = 0; k < 1000; k++) {
        heap_pop(&h, &item);
        if (item.priority != numbers[k]) {
            ok = 0;
        }
    }
    check(ok, "1000 random numbers come out in sorted order");

    heap_init(&h);
    ok = 1;
    for (int k = 0; k < HEAP_MAX; k++) {
        if (heap_push(&h, k, k) == 0) {
            ok = 0;
        }
    }
    check(ok, "can push 4096 items");
    check(heap_push(&h, 1, 1) == 0, "push into a full heap returns 0");

    printf("\n%d passed, %d failed\n", passed, failed);
    return failed == 0 ? 0 : 1;
}