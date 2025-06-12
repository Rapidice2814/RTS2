#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include <stdint.h>

#include "audio.h"




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

    // printf("All threads finished.\n");


    Sound_Init();

    while (1) {
       if(Sound_Loop()) break;
    }

    Sound_Deinit();

    printf("Program finished.\n");

    return 0;
}

