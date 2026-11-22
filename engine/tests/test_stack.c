#include <stdio.h>
#include "stack.h"

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

static StackItem make_item(int type, int cell_idx, int tower_type, int gold)
{
    StackItem item;
    item.type = type;
    item.cell_idx = cell_idx;
    item.tower_type = tower_type;
    item.gold = gold;
    return item;
}

int main(void)
{
    Stack s;
    StackItem item;
    int ok;

    check(stack_init(&s, 4) == 1, "init succeeds");
    check(stack_is_empty(&s), "new stack is empty");
    check(stack_pop(&s, &item) == 0, "pop on empty stack returns 0");
    check(stack_peek(&s, &item) == 0, "peek on empty stack returns 0");
    stack_free(&s);
    check(stack_init(&s, 0) == 0, "init with capacity 0 is rejected");

    stack_init(&s, 4);
    stack_push(&s, make_item(1, 10, 2, 50));
    stack_push(&s, make_item(1, 20, 3, 75));
    stack_push(&s, make_item(2, 10, 2, -50));
    check(!stack_is_empty(&s), "stack is not empty after pushes");

    check(stack_peek(&s, &item) == 1 && item.type == 2 && item.cell_idx == 10,
          "peek shows the last pushed item");
    check(stack_peek(&s, &item) == 1 && item.type == 2,
          "peek does not remove the item");

    ok = 1;
    if (!stack_pop(&s, &item) || item.type != 2 || item.cell_idx != 10 || item.gold != -50) ok = 0;
    if (!stack_pop(&s, &item) || item.type != 1 || item.cell_idx != 20 || item.gold != 75) ok = 0;
    if (!stack_pop(&s, &item) || item.type != 1 || item.cell_idx != 10 || item.gold != 50) ok = 0;
    check(ok, "pops come out in reverse order with all fields intact");
    check(stack_is_empty(&s), "stack is empty after popping everything");

    stack_free(&s);
    stack_init(&s, 2);
    ok = 1;
    for (int k = 0; k < 100; k++) {
        if (stack_push(&s, make_item(k, k * 2, k * 3, k * 4)) == 0) {
            ok = 0;
        }
    }
    check(ok, "100 pushes into capacity 2 all succeed");
    check(s.top == 99 && s.capacity >= 100, "stack grew to hold 100 items");
    ok = 1;
    for (int k = 99; k >= 0; k--) {
        if (stack_pop(&s, &item) == 0 || item.type != k || item.gold != k * 4) {
            ok = 0;
        }
    }
    check(ok, "100 items pop back in exact reverse order");

    stack_push(&s, make_item(5, 5, 5, 5));
    int old_capacity = s.capacity;
    stack_clear(&s);
    check(stack_is_empty(&s) && s.capacity == old_capacity, "clear empties but keeps capacity");

    stack_free(&s);
    check(s.items == NULL && s.top == -1 && s.capacity == 0, "free resets everything");

    check(stack_push(&s, make_item(7, 7, 7, 7)) == 1 && s.top == 0, "push after free starts a new block");
    stack_free(&s);

    printf("\n%d passed, %d failed\n", passed, failed);
    return failed == 0 ? 0 : 1;
}