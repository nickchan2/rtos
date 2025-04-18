#pragma once

#include "rtos.h"
#include "rtos_assert.h"

#include <stdbool.h>
#include <stddef.h>

static bool dll_is_empty(const rtos_dll_t *dll) {
    return dll->head == NULL;
}

static rtos_task_t *dll_pop_front(rtos_dll_t *dll) {
    ASSERT(dll->head != NULL);
    rtos_task_t *const popped = dll->head;
    if (popped->next == NULL) {
        dll->tail = NULL;
    } else {
        popped->next->prev = NULL;
    }
    dll->head = popped->next;
    popped->prev = NULL;
    popped->next = NULL;
    return popped;
}

static void dll_push_front(rtos_dll_t *dll, rtos_task_t *task) {
    if (dll->head == NULL) {
        dll->head = task;
        dll->tail = task;
        task->prev = NULL;
        task->next = NULL;
    } else {
        dll->head->prev = task;
        task->next = dll->head;
        dll->head = task;
        task->prev = NULL;
    }
}

static void dll_push_back(rtos_dll_t *dll, rtos_task_t *task) {
    if (dll->head == NULL) {
        dll->head = task;
        dll->tail = task;
        task->prev = NULL;
        task->next = NULL;
    } else {
        dll->tail->next = task;
        task->prev = dll->tail;
        task->next = NULL;
        dll->tail = task;
    }
}

static void dll_insert_ascending(rtos_dll_t *dll, rtos_task_t *task) {
    rtos_task_t *ptr = dll->head;
    while (ptr != NULL && ptr->wake_time < task->wake_time) {
        ptr = ptr->next;
    }
    if (ptr == NULL) {
        dll_push_back(dll, task);
    } else {
        if (ptr->prev == NULL) {
            dll->head = task;
        } else {
            ptr->prev->next = task;
        }
        task->prev = ptr->prev;
        task->next = ptr;
        ptr->prev = task;
    }
}
