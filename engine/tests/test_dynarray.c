#include <stdio.h>
#include "dynarray.h"

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

int main(void)
{
    IntArray a;
    int value;
    int ok;

    check(intarray_init(&a, 4) == 1, "init succeeds");
    check(a.size == 0 && a.capacity == 4, "new array is empty with capacity 4");
    intarray_free(&a);
    check(intarray_init(&a, 0) == 0, "init with capacity 0 is rejected");
    intarray_init(&a, 4);

    intarray_push(&a, 42);
    check(a.size == 1, "size is 1 after one push");
    check(intarray_get(&a, 0, &value) == 1 && value == 42, "get(0) returns 42");

    intarray_free(&a);
    intarray_init(&a, 2);
    ok = 1;
    for (int k = 0; k < 100; k++) {
        if (intarray_push(&a, k * 10) == 0) {
            ok = 0;
        }
    }
    check(ok, "100 pushes into capacity 2 all succeed");
    check(a.size == 100 && a.capacity >= 100, "array grew to hold 100 items");
    ok = 1;
    for (int k = 0; k < 100; k++) {
        if (intarray_get(&a, k, &value) == 0 || value != k * 10) {
            ok = 0;
        }
    }
    check(ok, "all 100 values survive the resizing");

    check(intarray_get(&a, -1, &value) == 0, "get(-1) is rejected");
    check(intarray_get(&a, a.size, &value) == 0, "get(size) is rejected");

    int old_capacity = a.capacity;
    intarray_clear(&a);
    check(a.size == 0 && a.capacity == old_capacity, "clear empties but keeps capacity");

    intarray_free(&a);
    check(a.data == NULL && a.size == 0 && a.capacity == 0, "free resets everything");

    printf("\n%d passed, %d failed\n", passed, failed);
    return failed == 0 ? 0 : 1;
}