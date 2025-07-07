#include "audioIO.h"

#include <math.h> 
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <alsa/asoundlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include <time.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif


#define CAPTURE_DEVICE "hw:4"
#define PLAYBACK_DEVICE "hw:1"
#define CAPTURE_CHANNELS 1
#define PLAYBACK_CHANNELS 2
#define SAMPLE_RATE 48000
#define FORMAT SND_PCM_FORMAT_S16_LE

#define PERIOD_SIZE (48*2) //should be multiple of 48
#define BUFFER_SIZE (PERIOD_SIZE * 4)
// #define PERIOD_SIZE 10000

snd_pcm_t *capture_handle, *playback_handle;
snd_pcm_hw_params_t *capture_hw_params, *playback_hw_params;


/*Functions*/
void process_sample(int16_t *in, int16_t *out, uint8_t capture_channel, uint8_t playback_channel);

/* Configures ALSA to get the samples*/
int Sound_Init(){
    int retval;
    /* Capture device */
    // if ((retval = snd_pcm_open(&capture_handle, CAPTURE_DEVICE, SND_PCM_STREAM_CAPTURE, SND_PCM_NONBLOCK)) < 0) {
    if ((retval = snd_pcm_open(&capture_handle, CAPTURE_DEVICE, SND_PCM_STREAM_CAPTURE, 0)) < 0) {
        fprintf(stderr, "Cannot open capture device: %s\n", snd_strerror(retval));
        return 1;
    }

    snd_pcm_hw_params_alloca(&capture_hw_params);
    snd_pcm_hw_params_any(capture_handle, capture_hw_params);
    snd_pcm_hw_params_set_access(capture_handle, capture_hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(capture_handle, capture_hw_params, FORMAT);
    snd_pcm_hw_params_set_channels(capture_handle, capture_hw_params, CAPTURE_CHANNELS);
    snd_pcm_hw_params_set_rate(capture_handle, capture_hw_params, SAMPLE_RATE, 0);
    snd_pcm_uframes_t capture_period_size = PERIOD_SIZE;
    snd_pcm_hw_params_set_period_size_near(capture_handle, capture_hw_params, &capture_period_size, 0);
    snd_pcm_uframes_t capture_buffer_size = BUFFER_SIZE;
    snd_pcm_hw_params_set_buffer_size_near(capture_handle, capture_hw_params, &capture_buffer_size);

    if ((retval = snd_pcm_hw_params(capture_handle, capture_hw_params)) < 0) {
        fprintf(stderr, "Cannot set hw params: %s\n", snd_strerror(retval));
        snd_pcm_close(capture_handle);
        snd_pcm_close(playback_handle);
        return 1;
    }

    snd_pcm_prepare(capture_handle);



    // if ((retval = snd_pcm_open(&playback_handle, PLAYBACK_DEVICE, SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK)) < 0) {
    if ((retval = snd_pcm_open(&playback_handle, PLAYBACK_DEVICE, SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
        fprintf(stderr, "Cannot open playback device: %s\n", snd_strerror(retval));
        snd_pcm_close(capture_handle);
        return 1;
    }

    snd_pcm_hw_params_alloca(&playback_hw_params);
    snd_pcm_hw_params_any(playback_handle, playback_hw_params);
    snd_pcm_hw_params_set_access(playback_handle, playback_hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(playback_handle, playback_hw_params, FORMAT);
    snd_pcm_hw_params_set_channels(playback_handle, playback_hw_params, PLAYBACK_CHANNELS);
    snd_pcm_hw_params_set_rate(playback_handle, playback_hw_params, SAMPLE_RATE, 0);

    snd_pcm_uframes_t playback_period_size = PERIOD_SIZE;
    snd_pcm_hw_params_set_period_size_near(playback_handle, playback_hw_params, &playback_period_size, 0);
    snd_pcm_uframes_t playback_buffer_size = BUFFER_SIZE;
    snd_pcm_hw_params_set_buffer_size_near(playback_handle, playback_hw_params, &playback_buffer_size);

    if ((retval = snd_pcm_hw_params(playback_handle, playback_hw_params)) < 0) {
        fprintf(stderr, "Cannot set hw params: %s\n", snd_strerror(retval));
        snd_pcm_close(capture_handle);
        snd_pcm_close(playback_handle);
        return 1;
    }


    snd_pcm_sw_params_t *swparams;
    snd_pcm_sw_params_alloca(&swparams);

    if ((retval = snd_pcm_sw_params_current(playback_handle, swparams)) < 0) {
        fprintf(stderr, "Cannot get current sw params: %s\n", snd_strerror(retval));
        return 1;
    }

    snd_pcm_uframes_t buffer_size, period_size;
    if ((retval = snd_pcm_get_params(playback_handle, &buffer_size, &period_size)) < 0) {
        fprintf(stderr, "Cannot get params: %s\n", snd_strerror(retval));
        return 1;
    }

    // Set start threshold to buffer size to delay start until buffer full
    if ((retval = snd_pcm_sw_params_set_start_threshold(playback_handle, swparams, buffer_size)) < 0) {
        fprintf(stderr, "Cannot set start threshold: %s\n", snd_strerror(retval));
        return 1;
    }

    // Set avail_min to period size for notifications on period availability
    if ((retval = snd_pcm_sw_params_set_avail_min(playback_handle, swparams, period_size)) < 0) {
        fprintf(stderr, "Cannot set avail_min: %s\n", snd_strerror(retval));
        return 1;
    }

    // Apply software parameters
    if ((retval = snd_pcm_sw_params(playback_handle, swparams)) < 0) {
        fprintf(stderr, "Cannot set sw params: %s\n", snd_strerror(retval));
        return 1;
    }

    // snd_pcm_writei(playback_handle, buffer, 400);

    snd_pcm_prepare(playback_handle);

    return 0;
}


/* Closes ALSA*/
int Sound_Deinit(){
    snd_pcm_close(capture_handle);
    snd_pcm_close(playback_handle);
}


static struct timespec start_capture, end_capture;
static audio_io_args_t *g_audio_args = NULL;

/*saves microphone to fifo*/
void* Function_Capture(void* arg) {
    fifo_t *fifo = (fifo_t *)arg;
    
    int frames_read;
    int16_t capture_buffer[PERIOD_SIZE * CAPTURE_CHANNELS];
    int16_t ch1_buffer[PERIOD_SIZE]; 
    
    
    while(1){
        clock_gettime(CLOCK_MONOTONIC, &start_capture);
        
        frames_read = snd_pcm_readi(capture_handle, capture_buffer, PERIOD_SIZE); 
        if(frames_read < 0) {
            if(frames_read == -EPIPE) {
                // overrun recovery
                printf("Capture error: %s\n", snd_strerror(frames_read));
                snd_pcm_prepare(capture_handle);
                continue;
            } else {
                printf("Capture error: %s\n", snd_strerror(frames_read));
                snd_pcm_prepare(capture_handle);
                continue;
            }
        }else{
            // printf("Read: %d\n", frames_read);
        }
        for (int i = 0; i < frames_read; i++) {
            ch1_buffer[i] = capture_buffer[i * CAPTURE_CHANNELS];  // channel 1 sample
        }
        
        fifo_push_batch(fifo, ch1_buffer, frames_read);
        
        clock_gettime(CLOCK_MONOTONIC, &end_capture);
        double elapsed = ((end_capture.tv_sec - start_capture.tv_sec) + (end_capture.tv_nsec - start_capture.tv_nsec) / 1e9) * 1000000;
        // printf("Cap Time: %fus\n", elapsed);
    }
    return NULL;
}


static struct timespec start_playback, end_playback;
/*takes fifo and outputs it to speaker*/
void* Function_Playback(void* arg) {
    fifo_t *fifo = (fifo_t *)arg;

    int16_t ch1_buffer[PERIOD_SIZE]; 
    int16_t playback_buffer[PERIOD_SIZE * PLAYBACK_CHANNELS];


    while(1){
        clock_gettime(CLOCK_MONOTONIC, &start_playback);

        fifo_pop_batch(fifo, ch1_buffer, PERIOD_SIZE);

        for (int i = 0; i < PERIOD_SIZE; i++) {
            int16_t sample = ch1_buffer[i];
            for (int ch = 0; ch < PLAYBACK_CHANNELS; ch++) {
                playback_buffer[i * PLAYBACK_CHANNELS + ch] = sample;
            }
        }


        int framess_written = snd_pcm_writei(playback_handle, playback_buffer, PERIOD_SIZE);
        if(framess_written < 0) {
            if(framess_written == -EPIPE) {
                printf("Playback error: %s\n", snd_strerror(framess_written));
                snd_pcm_prepare(playback_handle);
                continue; // Not enough space
            } else {
                printf("Playback error: %s\n", snd_strerror(framess_written));
                snd_pcm_prepare(playback_handle);
                continue;
            }
        }else{
            // printf("Written: %d\n", framess_written);
        }

        fifo_try_push_batch(g_audio_args->echo_fifo, ch1_buffer, framess_written);

        clock_gettime(CLOCK_MONOTONIC, &end_playback);
        double elapsed = ((end_playback.tv_sec - start_playback.tv_sec) + (end_playback.tv_nsec - start_playback.tv_nsec) / 1e9) * 1000000;
        // printf("Play Time: %fus\n", elapsed);
    }
    return NULL;
}

void* Function_AudioIO(void* arg) {
    g_audio_args = (audio_io_args_t *)arg;

    printf("FIFO cap: %p, FIFO play: %p\n", 
            g_audio_args->capture_fifo,
            g_audio_args->playback_fifo);

    Sound_Init();

    pthread_t thread_capture, thread_playback;
    pthread_create(&thread_capture, NULL, Function_Capture, g_audio_args->capture_fifo);
    pthread_create(&thread_playback, NULL, Function_Playback, g_audio_args->playback_fifo);

    pthread_join(thread_capture, NULL);
    pthread_join(thread_playback, NULL);

    Sound_Deinit();
    return NULL;
}



