#pragma once

#include "rtos.h"
#include "rtos_assert.h"

#include <stdbool.h>
#include <stddef.h>

static bool tlist_is_empty(const rtos_tlist_t *tlist) {
    return tlist->head == NULL;
}

static rtos_tcb_t *tlist_pop_front(rtos_tlist_t *tlist) {
    ASSERT(tlist->head != NULL);
    rtos_tcb_t *const popped = tlist->head;
    if (popped->next == NULL) {
        tlist->tail = NULL;
    } else {
        popped->next->prev = NULL;
    }
    tlist->head = popped->next;
    popped->prev = NULL;
    popped->next = NULL;
    return popped;
}

static void tlist_push_front(rtos_tlist_t *tlist, rtos_tcb_t *task) {
    if (tlist->head == NULL) {
        tlist->head = task;
        tlist->tail = task;
        task->prev = NULL;
        task->next = NULL;
    } else {
        tlist->head->prev = task;
        task->next = tlist->head;
        tlist->head = task;
        task->prev = NULL;
    }
}

static void tlist_push_back(rtos_tlist_t *tlist, rtos_tcb_t *task) {
    if (tlist->head == NULL) {
        tlist->head = task;
        tlist->tail = task;
        task->prev = NULL;
        task->next = NULL;
    } else {
        tlist->tail->next = task;
        task->prev = tlist->tail;
        task->next = NULL;
        tlist->tail = task;
    }
}

static void tlist_insert_ascending(rtos_tlist_t *tlist, rtos_tcb_t *task) {
    rtos_tcb_t *ptr = tlist->head;
    while (ptr != NULL && ptr->wake_time < task->wake_time) {
        ptr = ptr->next;
    }
    if (ptr == NULL) {
        tlist_push_back(tlist, task);
    } else {
        if (ptr->prev == NULL) {
            tlist->head = task;
        } else {
            ptr->prev->next = task;
        }
        task->prev = ptr->prev;
        task->next = ptr;
        ptr->prev = task;
    }
}
