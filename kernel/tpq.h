#pragma once

#include "rtos.h"
#include "rtos_assert.h"
#include "tlist.h"

typedef struct {
    rtos_tlist_t tlists[RTOS_NUM_PRIORITY_LEVELS];
} tpq_t;

// static void tpq_init(tpq_t *tpq) {
//     for (int i = 0; i < RTOS_NUM_PRIORITY_LEVELS; ++i) {
//         tpq->tlists[i].head = NULL;
//         tpq->tlists[i].tail = NULL;
//     }
// }

static bool tpq_list_is_empty(tpq_t *tpq, rtos_tcb_t *task) {
    return tlist_is_empty(&tpq->tlists[task->priority]);
}

static void tpq_push_front(tpq_t *tpq, rtos_tcb_t *task) {
    tlist_push_front(&tpq->tlists[task->priority], task);
}

static void tpq_push_back(tpq_t *tpq, rtos_tcb_t *task) {
    tlist_push_back(&tpq->tlists[task->priority], task);
}

static rtos_tcb_t *tpq_pop_front(tpq_t *tpq) {
    rtos_tcb_t *task = NULL;
    for (int i = RTOS_NUM_PRIORITY_LEVELS - 1; i >= 0; --i) {
        if (!tlist_is_empty(&tpq->tlists[i])) {
            task = tlist_pop_front(&tpq->tlists[i]);
            break;
        }
    }
    return task;
}
