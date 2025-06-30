#include "fifo.h"

#include <stdlib.h>
#include <string.h>

int fifo_init(fifo_t *fifo, int capacity, size_t element_size) {
    fifo->capacity = capacity;
    fifo->element_size = element_size;
    fifo->buffer = malloc(capacity * element_size);
    if (!fifo->buffer) return -1;

    fifo->head = fifo->tail = fifo->count = 0;
    pthread_mutex_init(&fifo->mutex, NULL);
    pthread_cond_init(&fifo->not_full, NULL);
    pthread_cond_init(&fifo->not_empty, NULL);
    return 0;
}


int fifo_push(fifo_t *fifo, const void *item) {
    pthread_mutex_lock(&fifo->mutex);
    while (fifo->count == fifo->capacity) {
        pthread_cond_wait(&fifo->not_full, &fifo->mutex);
    }

    void *dest = (char *)fifo->buffer + (fifo->tail * fifo->element_size);
    memcpy(dest, item, fifo->element_size);
    fifo->tail = (fifo->tail + 1) % fifo->capacity;
    fifo->count++;

    pthread_cond_signal(&fifo->not_empty);
    pthread_mutex_unlock(&fifo->mutex);
    return 0;
}

int fifo_pop(fifo_t *fifo, void *item) {
    pthread_mutex_lock(&fifo->mutex);
    while (fifo->count == 0) {
        pthread_cond_wait(&fifo->not_empty, &fifo->mutex);
    }

    void *src = (char *)fifo->buffer + (fifo->head * fifo->element_size);
    memcpy(item, src, fifo->element_size);
    fifo->head = (fifo->head + 1) % fifo->capacity;
    fifo->count--;

    pthread_cond_signal(&fifo->not_full);
    pthread_mutex_unlock(&fifo->mutex);
    return 0;
}

int fifo_push_batch(fifo_t *fifo, const void *src_array, int n) {
    pthread_mutex_lock(&fifo->mutex);

    // Wait until there's room for all n items
    while (fifo->count + n > fifo->capacity) {
        pthread_cond_wait(&fifo->not_full, &fifo->mutex);
    }

    for (int i = 0; i < n; ++i) {
        const void *src = (const char *)src_array + (i * fifo->element_size);
        void *dst = (char *)fifo->buffer + (fifo->tail * fifo->element_size);
        memcpy(dst, src, fifo->element_size);
        fifo->tail = (fifo->tail + 1) % fifo->capacity;
        fifo->count++;
    }

    pthread_cond_signal(&fifo->not_empty);
    pthread_mutex_unlock(&fifo->mutex);
    return n;
}

int fifo_pop_batch(fifo_t *fifo, void *dest_array, int n) {
    pthread_mutex_lock(&fifo->mutex);

    // Wait until enough items are available
    while (fifo->count < n) {
        pthread_cond_wait(&fifo->not_empty, &fifo->mutex);
    }

    for (int i = 0; i < n; ++i) {
        void *src = (char *)fifo->buffer + (fifo->head * fifo->element_size);
        void *dst = (char *)dest_array + (i * fifo->element_size);
        memcpy(dst, src, fifo->element_size);
        fifo->head = (fifo->head + 1) % fifo->capacity;
        fifo->count--;
    }

    pthread_cond_signal(&fifo->not_full);
    pthread_mutex_unlock(&fifo->mutex);
    return n;
}

int fifo_try_push_batch(fifo_t *fifo, const void *src_array, int n) {
    pthread_mutex_lock(&fifo->mutex);

    // Check if there's enough room for all n items, return 0 if not
    if (fifo->count + n > fifo->capacity) {
        pthread_mutex_unlock(&fifo->mutex);
        return 0; // Not enough space, fail immediately
    }

    for (int i = 0; i < n; ++i) {
        const void *src = (const char *)src_array + (i * fifo->element_size);
        void *dst = (char *)fifo->buffer + (fifo->tail * fifo->element_size);
        memcpy(dst, src, fifo->element_size);
        fifo->tail = (fifo->tail + 1) % fifo->capacity;
        fifo->count++;
    }

    pthread_cond_signal(&fifo->not_empty);
    pthread_mutex_unlock(&fifo->mutex);
    return n;  // Number of items successfully pushed (always n here)
}

int fifo_try_pop_batch(fifo_t *fifo, void *dest_array, int n) {
    pthread_mutex_lock(&fifo->mutex);

    // Check if enough items are available, return 0 if not
    if (fifo->count < n) {
        pthread_mutex_unlock(&fifo->mutex);
        return 0;  // Not enough data available, fail immediately
    }

    for (int i = 0; i < n; ++i) {
        void *src = (char *)fifo->buffer + (fifo->head * fifo->element_size);
        void *dst = (char *)dest_array + (i * fifo->element_size);
        memcpy(dst, src, fifo->element_size);
        fifo->head = (fifo->head + 1) % fifo->capacity;
        fifo->count--;
    }

    pthread_cond_signal(&fifo->not_full);
    pthread_mutex_unlock(&fifo->mutex);
    return n;  // Number of items popped (always n here)
}




void fifo_destroy(fifo_t *fifo) {
    free(fifo->buffer);
    pthread_mutex_destroy(&fifo->mutex);
    pthread_cond_destroy(&fifo->not_full);
    pthread_cond_destroy(&fifo->not_empty);
}