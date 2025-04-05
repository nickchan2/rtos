#pragma once

#include "rtos.h"
#include "rtos_assert.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

static bool queue_is_empty(const rtos_mqueue_t *queue) {
    return queue->head == queue->tail && !queue->is_full;
}

static bool queue_is_full(const rtos_mqueue_t *queue) {
    return queue->is_full;
}

static void queue_enqueue(rtos_mqueue_t *queue, const void *data) {
    ASSERT(!queue_is_full(queue));
    const size_t index = queue->head * queue->slot_size;
    for (size_t i = 0; i < queue->slot_size; ++i) {
        queue->data[index + i] = ((uint8_t *)data)[i];
    }
    queue->head = (queue->head + 1) % queue->slots;
    if (queue->head == queue->tail) {
        queue->is_full = true;
    }
}

static void queue_dequeue(rtos_mqueue_t *queue, void *data) {
    ASSERT(!queue_is_empty(queue));
    const size_t index = queue->tail * queue->slot_size;
    for (size_t i = 0; i < queue->slot_size; ++i) {
        ((uint8_t *)data)[i] = queue->data[index + i];
    }
    queue->tail = (queue->tail + 1) % queue->slots;
    queue->is_full = false;
}
