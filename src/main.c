#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include <stdint.h>

#include "audio.h"
#include "fifo.h"




// Thread function: what each thread will run
void* thread_function(void* arg) {
    int thread_num = *((int*)arg);
    printf("Hello from thread %d!\n", thread_num);
    return NULL;
}




int main(void) {
    const int NUM_THREADS = 5;
    pthread_t threads[NUM_THREADS];
    int thread_args[NUM_THREADS];

    // Create threads
    for (int i = 0; i < NUM_THREADS; i++) {
        thread_args[i] = i + 1;
        int rc = pthread_create(&threads[i], NULL, thread_function, &thread_args[i]);
        if (rc) {
            fprintf(stderr, "Error creating thread %d\n", i+1);
            exit(EXIT_FAILURE);
        }
    }

    // Wait for all threads to complete
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("Test threads finished.\n");

    fifo_t fifo_in, fifo_out;

    fifo_init(&fifo_in, 128, sizeof(int16_t));   // Adjust size/type as needed
    fifo_init(&fifo_out, 128, sizeof(int16_t));
    audio_io_args_t *audio_io_args = malloc(sizeof(audio_io_args_t));
    audio_io_args->input_fifo = &fifo_in;
    audio_io_args->output_fifo = &fifo_out;

    pthread_t audioIO_thread;
    pthread_create(&audioIO_thread, NULL, Function_AudioIO, (void *)audio_io_args);
    pthread_join(audioIO_thread, NULL);
    

    printf("Program finished.\n");

    return 0;
}

