#include "audio.h"

#include <math.h> 
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <alsa/asoundlib.h>
#include <string.h>
#include <unistd.h>

#include <time.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif


#define CAPTURE_DEVICE "hw:4"
#define PLAYBACK_DEVICE "hw:3"
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
    if ((retval = snd_pcm_open(&capture_handle, CAPTURE_DEVICE, SND_PCM_STREAM_CAPTURE, SND_PCM_NONBLOCK)) < 0) {
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



    if ((retval = snd_pcm_open(&playback_handle, PLAYBACK_DEVICE, SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK)) < 0) {
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

struct timespec start, end;

int16_t capture_buffer[PERIOD_SIZE * CAPTURE_CHANNELS];
int16_t playback_buffer[PERIOD_SIZE * PLAYBACK_CHANNELS];

/* Loop for the sound processing*/
int Sound_Loop(){
    int available_read = snd_pcm_avail(capture_handle);
    if(available_read > PERIOD_SIZE) {
        printf("READ available frames: %d, Out of: %d\n", available_read, BUFFER_SIZE); //should be close to 0
    }

    int frames_read = snd_pcm_readi(capture_handle, capture_buffer, PERIOD_SIZE); 
    if(frames_read < 0) {
        if(frames_read == -EAGAIN) {
            return 0; // No new data available
        } else {
            printf("Capture error: %s\n", snd_strerror(frames_read));
            snd_pcm_prepare(capture_handle);
        }
    }else{
        // printf("Read: %d\n", frames_read);
    }

    clock_gettime(CLOCK_MONOTONIC, &start);

    for (int i = 0; i < frames_read; i++) {
        process_sample(&capture_buffer[i * CAPTURE_CHANNELS], &playback_buffer[i * PLAYBACK_CHANNELS], CAPTURE_CHANNELS, PLAYBACK_CHANNELS);
    }
    usleep(1500);

    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed = ((end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9) * 1000000;
    // printf("Time: %fus\n", elapsed);


    int available_write = snd_pcm_avail(playback_handle);
    if(available_write < PERIOD_SIZE) {
        printf("WRITE available frames: %d, Out of: %d\n", available_write, BUFFER_SIZE); //should be close to BUFFER_SIZE
    }

    int framess_written = snd_pcm_writei(playback_handle, playback_buffer, frames_read);
    if(framess_written < 0) {
        if(framess_written == -EAGAIN) {
            return 0; // Not enough space
        } else {
            printf("Playback error: %s\n", snd_strerror(framess_written));
            snd_pcm_prepare(playback_handle);
        }
    }else{
        // printf("Written: %d\n", framess_written);
    } 
    
    return 0;
}

/* Closes ALSA*/
int Sound_Deinit(){
    snd_pcm_close(capture_handle);
    snd_pcm_close(playback_handle);
}












int16_t multiply_and_clip(int16_t value, int factor);

// Example processing function (copy input to output)
void process_sample(int16_t *in, int16_t *out, uint8_t capture_channel, uint8_t playback_channel) {
    // printf("In: %d %d\n", in[0], in[1]);
    if(capture_channel == 1 && playback_channel == 2) {
        out[0] = multiply_and_clip(in[0], 10); // Left channel
        out[1] = multiply_and_clip(in[0], 10); // Right channel
        // printf("In: %d\n", in[0]);
    } else if(capture_channel == 2 && playback_channel == 2) {
        out[0] = in[0]; // Left channel
        out[1] = in[1]; // Right channel
    } else if(capture_channel == 1 && playback_channel == 1) {
        out[0] = in[0];
    } 
    // printf("In: %6d %6d, Out: %6d, %6d\n", in[0], in[1], out[0], out[1]);
}


int16_t multiply_and_clip(int16_t value, int factor) {
    int32_t result = (int32_t)value * factor;  // Use wider type to avoid overflow
    
    if (result > 32767) {
        return 32767;
    } else if (result < -32768) {
        return -32768;
    }
    return (int16_t)result;
}


#define PI 3.14159265358979323846
#define FREQUENCY 440.0f  // A4 note
#define AMPLITUDE 3000    // 16-bit range: -32768 to 32767

void sine(int16_t *in, int16_t *out, uint8_t capture_channel, uint8_t playback_channel) {
    static double phase = 0.0;
    double increment = 2.0 * PI * FREQUENCY / SAMPLE_RATE;
    
    int16_t sample = (int16_t)(sin(phase) * AMPLITUDE);
    phase += increment;
    if (phase >= 2.0 * PI) {
        phase -= 2.0 * PI;
    }

    // Output sine to all configured playback channels
    if (playback_channel == 2) {
        out[0] = sample;  // Left
        out[1] = sample;  // Right
    } else if (playback_channel == 1) {
        out[0] = sample;
    }
}