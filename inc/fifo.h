#pragma once

#include <stdlib.h>
#include <pthread.h>

// FIFO structure definition
typedef struct {
    void *buffer;            // Pointer to the data buffer
    int capacity;            // Maximum number of elements
    int count;               // Current number of elements
    int head;                // Read index
    int tail;                // Write index
    size_t element_size;     // Size of each element in bytes
    pthread_mutex_t mutex;   // Mutex for thread safety
    pthread_cond_t not_full; // Condition variable for buffer not full
    pthread_cond_t not_empty;// Condition variable for buffer not empty
} fifo_t;

// Function declarations
int fifo_init(fifo_t *fifo, int capacity, size_t element_size);
void fifo_destroy(fifo_t *fifo);
int fifo_push(fifo_t *fifo, const void *item);
int fifo_pop(fifo_t *fifo, void *item);
int fifo_push_batch(fifo_t *fifo, const void *src_array, int n);
int fifo_pop_batch(fifo_t *fifo, void *dest_array, int n);
int fifo_try_push_batch(fifo_t *fifo, const void *src_array, int n);
int fifo_try_pop_batch(fifo_t *fifo, void *dest_array, int n);

