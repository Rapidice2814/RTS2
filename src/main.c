#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>


#include <stdint.h>

#include "audioIO.h"
#include "fifo.h"
#include "audioProcessing.h"
#include "UI.h"




// Thread function: what each thread will run
void* thread_function(void* arg) {
    int thread_num = *((int*)arg);
    printf("Hello from thread %d!\n", thread_num);
    return NULL;
}




int main(void) {

    fifo_t audio_io_fifo_capture, audio_io_fifo_playback;
    fifo_t echo_fifo;
    fifo_t audio_fifo1;

    fifo_init(&audio_io_fifo_capture, 256, sizeof(int16_t));   // Adjust size/type as needed
    fifo_init(&audio_io_fifo_playback, 256, sizeof(int16_t));
    fifo_init(&echo_fifo, 256, sizeof(int16_t)); // Echo FIFO for audio processing
    fifo_init(&audio_fifo1, 256, sizeof(int16_t)); // Example FIFO for audio processing

    audio_io_args_t *audio_io_args = malloc(sizeof(audio_io_args_t));
    audio_io_args->capture_fifo = &audio_io_fifo_capture;
    audio_io_args->playback_fifo = &audio_io_fifo_playback;
    audio_io_args->echo_fifo = &echo_fifo;

    //UI
    UIState ui_state = {
        .denoise = true,
        .agc = true,
        .dereverb = false,
        .echo = true
    };

    audio_processing_args_t *audio_processing_args = malloc(sizeof(audio_processing_args_t));
    audio_processing_args->in_fifo = &audio_io_fifo_capture;
    audio_processing_args->out_fifo = &audio_fifo1;
    audio_processing_args->echo_fifo = &echo_fifo;
    audio_processing_args->ui_state = &ui_state;

    audio_simple_node_args_t *audio_volume_leveler_args = malloc(sizeof(audio_simple_node_args_t));
    audio_volume_leveler_args->in_fifo = &audio_fifo1; // Input FIFO for volume leveler
    audio_volume_leveler_args->out_fifo = &audio_io_fifo_playback;
    audio_volume_leveler_args->ui_state = &ui_state;

    pthread_t audio_processing_thread;
    pthread_create(&audio_processing_thread, NULL, Function_Audio_Echo_Cancelling, (void *)audio_processing_args);

    pthread_t audio_volume_thread;
    pthread_create(&audio_processing_thread, NULL, Function_Audio_Volume_Leveler, (void *)audio_volume_leveler_args);

    pthread_t audioIO_thread;
    pthread_create(&audioIO_thread, NULL, Function_AudioIO, (void *)audio_io_args);

    
    pthread_mutex_init(&ui_state.lock, NULL);
    pthread_t ui_thread;
    pthread_create(&ui_thread, NULL, ui_thread_func, &ui_state);


    pthread_t audio_settings_thread;
    pthread_create(&ui_thread, NULL, AudioSettings, &ui_state);





    pthread_join(audio_processing_thread, NULL);
    pthread_join(audio_volume_thread, NULL);
    pthread_join(audioIO_thread, NULL);
    pthread_join(ui_thread, NULL);
    pthread_join(audio_settings_thread, NULL);
    pthread_mutex_destroy(&ui_state.lock);
    

    printf("Program finished.\n");

    return 0;
}

