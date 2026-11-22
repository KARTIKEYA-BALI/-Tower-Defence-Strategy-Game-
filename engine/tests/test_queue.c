#include <stdio.h>
#include "queue.h"

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

static Event make_event(int type, int entity_id, int cell, int value) {
    Event e;
    e.type = type;
    e.entity_id = entity_id;
    e.cell = cell;
    e.value = value;
    return e;
}

int main(void) {
    Queue q;
    Event out;

    check(queue_init(&q, 0) == 0, "init rejects capacity 0");
    check(queue_init(&q, -3) == 0, "init rejects negative capacity");

    check(queue_init(&q, 4) == 1, "init capacity 4 succeeds");
    check(queue_is_empty(&q), "new queue is empty");
    check(!queue_is_full(&q), "new queue is not full");
    check(queue_pop(&q, &out) == 0, "pop on empty queue fails");
    check(queue_peek(&q, &out) == 0, "peek on empty queue fails");

    check(queue_push(&q, make_event(1, 10, 5, 100)) == 1, "push first event");
    check(!queue_is_empty(&q), "queue not empty after one push");
    check(queue_peek(&q, &out) == 1 && out.entity_id == 10, "peek sees first event");
    check(queue_peek(&q, &out) == 1 && out.entity_id == 10, "peek twice does not remove");
    check(queue_pop(&q, &out) == 1 && out.entity_id == 10 && out.type == 1
          && out.cell == 5 && out.value == 100, "pop returns all fields of the event");
    check(queue_is_empty(&q), "empty again after popping the only event");

    queue_push(&q, make_event(0, 1, 0, 0));
    queue_push(&q, make_event(0, 2, 0, 0));
    queue_push(&q, make_event(0, 3, 0, 0));
    queue_pop(&q, &out);
    check(out.entity_id == 1, "FIFO: first pop gives 1");
    queue_pop(&q, &out);
    check(out.entity_id == 2, "FIFO: second pop gives 2");
    queue_pop(&q, &out);
    check(out.entity_id == 3, "FIFO: third pop gives 3");
    check(queue_is_empty(&q), "empty after popping everything");

    queue_push(&q, make_event(0, 1, 0, 0));
    queue_push(&q, make_event(0, 2, 0, 0));
    queue_push(&q, make_event(0, 3, 0, 0));
    queue_push(&q, make_event(0, 4, 0, 0));
    check(queue_is_full(&q), "full after 4 pushes into capacity 4");
    check(queue_push(&q, make_event(0, 99, 0, 0)) == 0, "push on full queue fails");
    queue_peek(&q, &out);
    check(out.entity_id == 1, "failed push did not overwrite the front");

    queue_pop(&q, &out);
    queue_pop(&q, &out);
    check(!queue_is_full(&q), "not full after two pops");
    check(queue_push(&q, make_event(0, 5, 0, 0)) == 1, "push 5 wraps rear to slot 0");
    check(queue_push(&q, make_event(0, 6, 0, 0)) == 1, "push 6 wraps rear to slot 1");
    check(queue_is_full(&q), "full again after wrapping");

    queue_pop(&q, &out);
    check(out.entity_id == 3, "wrap: pop gives 3");
    queue_pop(&q, &out);
    check(out.entity_id == 4, "wrap: pop gives 4");
    queue_pop(&q, &out);
    check(out.entity_id == 5, "wrap: front wrapped, pop gives 5");
    queue_pop(&q, &out);
    check(out.entity_id == 6, "wrap: pop gives 6");
    check(queue_is_empty(&q), "empty after draining the wrapped queue");

    for (int round = 0; round < 10; round++) {
        queue_push(&q, make_event(0, round, 0, 0));
        queue_pop(&q, &out);
    }
    check(out.entity_id == 9 && queue_is_empty(&q), "push/pop 10 times around the ring");

    queue_push(&q, make_event(0, 7, 0, 0));
    queue_push(&q, make_event(0, 8, 0, 0));
    queue_clear(&q);
    check(queue_is_empty(&q), "clear empties the queue");
    check(queue_push(&q, make_event(0, 42, 0, 0)) == 1, "push works after clear");
    queue_pop(&q, &out);
    check(out.entity_id == 42, "pop after clear gives the new event");

    queue_free(&q);
    check(q.items == NULL && q.capacity == 0, "free sets items NULL and capacity 0");

    check(queue_init(&q, 1) == 1, "init capacity 1");
    check(queue_push(&q, make_event(0, 11, 0, 0)) == 1, "capacity 1: first push works");
    check(queue_push(&q, make_event(0, 12, 0, 0)) == 0, "capacity 1: second push fails");
    queue_pop(&q, &out);
    check(out.entity_id == 11 && queue_is_empty(&q), "capacity 1: pop gives 11, then empty");
    queue_free(&q);

    printf("\n%d passed, %d failed\n", passed, failed);
    return failed == 0 ? 0 : 1;
}