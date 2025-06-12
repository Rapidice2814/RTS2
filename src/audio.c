#include "audio.h"

#include <math.h> 
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <alsa/asoundlib.h>
#include <string.h>
#include <unistd.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif


#define CAPTURE_DEVICE "hw:3"
#define PLAYBACK_DEVICE "hw:3"
#define CAPTURE_CHANNELS 2
#define PLAYBACK_CHANNELS 2
#define SAMPLE_RATE 48000
#define FORMAT SND_PCM_FORMAT_S16_LE

#define PERIOD_SIZE 64
// #define PERIOD_SIZE 10000

snd_pcm_t *capture_handle, *playback_handle;
snd_pcm_hw_params_t *capture_hw_params, *playback_hw_params;

/*Functions*/
void process_sample(int16_t *in, int16_t *out, uint8_t capture_channel, uint8_t playback_channel);


/* Configures ALSA to get the samples*/
int Sound_Init(){
    int retval;

    /* Capture device */
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

    if ((retval = snd_pcm_hw_params(capture_handle, capture_hw_params)) < 0) {
        fprintf(stderr, "Cannot set hw params: %s\n", snd_strerror(retval));
        snd_pcm_close(capture_handle);
        snd_pcm_close(playback_handle);
        return 1;
    }

    snd_pcm_prepare(capture_handle);



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

    if ((retval = snd_pcm_hw_params(playback_handle, playback_hw_params)) < 0) {
        fprintf(stderr, "Cannot set hw params: %s\n", snd_strerror(retval));
        snd_pcm_close(capture_handle);
        snd_pcm_close(playback_handle);
        return 1;
    }

    snd_pcm_prepare(playback_handle);

    return 0;
}


int16_t capture_buffer[PERIOD_SIZE * CAPTURE_CHANNELS];
int16_t playback_buffer[PERIOD_SIZE * PLAYBACK_CHANNELS];

/* Loop for the sound processing*/
int Sound_Loop(){
    int retval;
    retval = snd_pcm_readi(capture_handle, capture_buffer, PERIOD_SIZE); 
    //this gets the amount samples from ALSA's internal buffer. In case the internal buffer does not have enough, it blocks the loop. The function returns how many frames it has read.
    if (retval < 0) {
        if (retval == -EPIPE) {
            snd_pcm_prepare(capture_handle);
            fprintf(stderr, "ALSA Capture buffer overflow\n");
        } else {
            fprintf(stderr, "Capture error: %s\n", snd_strerror(retval));
            return -1;
        }
    } else if (retval != PERIOD_SIZE) {
        fprintf(stderr, "Short read, expected %d frames, got %d\n", PERIOD_SIZE, retval);
    } else{
        
        // Process each sample pair (frame) individually
        for (int i = 0; i < retval; i++) {
            process_sample(&capture_buffer[i * CAPTURE_CHANNELS], &playback_buffer[i * PLAYBACK_CHANNELS], CAPTURE_CHANNELS, PLAYBACK_CHANNELS);
        }

        retval = snd_pcm_writei(playback_handle, playback_buffer, PERIOD_SIZE);
        if (retval < 0) {
            if (retval == -EPIPE) {
                snd_pcm_prepare(playback_handle);
                fprintf(stderr, "ALSA Playback buffer overflow\n");
            } else {
                fprintf(stderr, "Playback error: %s\n", snd_strerror(retval));
                return -1;
            }
        } else if (retval != PERIOD_SIZE) {
            fprintf(stderr, "Short write, expected %d frames, got %d\n", PERIOD_SIZE, retval);
        }
    }
    
    return 0;
}

/* Closes ALSA*/
int Sound_Deinit(){
    snd_pcm_close(capture_handle);
    snd_pcm_close(playback_handle);
}


// Example processing function (copy input to output)
void process_sample(int16_t *in, int16_t *out, uint8_t capture_channel, uint8_t playback_channel) {
    // printf("In: %d %d\n", in[0], in[1]);
    for (int i = 0; i < capture_channel; i++){
        out[i] = in[i];
    }
    // printf("In: %6d %6d, Out: %6d, %6d\n", in[0], in[1], out[0], out[1]);
}
