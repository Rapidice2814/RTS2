#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include <stdint.h>

#include "audioIO.h"
#include "fifo.h"
#include "audioProcessing.h"




// Thread function: what each thread will run
void* thread_function(void* arg) {
    int thread_num = *((int*)arg);
    printf("Hello from thread %d!\n", thread_num);
    return NULL;
}




int main(void) {
    // const int NUM_THREADS = 5;
    // pthread_t threads[NUM_THREADS];
    // int thread_args[NUM_THREADS];

    // // Create threads
    // for (int i = 0; i < NUM_THREADS; i++) {
    //     thread_args[i] = i + 1;
    //     int rc = pthread_create(&threads[i], NULL, thread_function, &thread_args[i]);
    //     if (rc) {
    //         fprintf(stderr, "Error creating thread %d\n", i+1);
    //         exit(EXIT_FAILURE);
    //     }
    // }

    // // Wait for all threads to complete
    // for (int i = 0; i < NUM_THREADS; i++) {
    //     pthread_join(threads[i], NULL);
    // }

    // printf("Test threads finished.\n");

    fifo_t audio_io_fifo_capture, audio_io_fifo_playback;

    fifo_init(&audio_io_fifo_capture, 256, sizeof(int16_t));   // Adjust size/type as needed
    fifo_init(&audio_io_fifo_playback, 256, sizeof(int16_t));
    audio_io_args_t *audio_io_args = malloc(sizeof(audio_io_args_t));
    audio_io_args->capture_fifo = &audio_io_fifo_capture;
    audio_io_args->playback_fifo = &audio_io_fifo_playback;

    audio_processing_args_t *audio_processing_args = malloc(sizeof(audio_processing_args_t));
    audio_processing_args->in_fifo = &audio_io_fifo_capture;
    audio_processing_args->out_fifo = &audio_io_fifo_playback;

    pthread_t audio_processing_thread;
    pthread_create(&audio_processing_thread, NULL, Function_Audio_Processing, (void *)audio_processing_args);

    pthread_t audioIO_thread;
    pthread_create(&audioIO_thread, NULL, Function_AudioIO, (void *)audio_io_args);

    pthread_join(audio_processing_thread, NULL);
    pthread_join(audioIO_thread, NULL);
    

    printf("Program finished.\n");

    return 0;
}

