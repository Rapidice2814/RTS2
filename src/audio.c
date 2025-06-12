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
#define SAMPLE_RATE 48000
#define CHANNELS 2
#define FORMAT SND_PCM_FORMAT_S16_LE

#define PERIOD_SIZE 256
// #define PERIOD_SIZE 10000

snd_pcm_t *capture_handle, *playback_handle;
snd_pcm_hw_params_t *hw_params;

/*Functions*/
void process_sample(int16_t *in, int16_t *out);


/* Configures ALSA to get the samples*/
int Sound_Init(){
    int retval;

    if ((retval = snd_pcm_open(&capture_handle, CAPTURE_DEVICE, SND_PCM_STREAM_CAPTURE, 0)) < 0) {
        fprintf(stderr, "Cannot open capture device: %s\n", snd_strerror(retval));
        return 1;
    }

    if ((retval = snd_pcm_open(&playback_handle, PLAYBACK_DEVICE, SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
        fprintf(stderr, "Cannot open playback device: %s\n", snd_strerror(retval));
        snd_pcm_close(capture_handle);
        return 1;
    }

    for (int i = 0; i < 2; i++) {
        snd_pcm_t *handle = (i == 0) ? capture_handle : playback_handle;

        snd_pcm_hw_params_alloca(&hw_params);
        snd_pcm_hw_params_any(handle, hw_params);
        snd_pcm_hw_params_set_access(handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);
        snd_pcm_hw_params_set_format(handle, hw_params, FORMAT);
        snd_pcm_hw_params_set_channels(handle, hw_params, CHANNELS);
        snd_pcm_hw_params_set_rate(handle, hw_params, SAMPLE_RATE, 0);

        snd_pcm_uframes_t period_size = PERIOD_SIZE;
        snd_pcm_hw_params_set_period_size_near(handle, hw_params, &period_size, 0);

        if ((retval = snd_pcm_hw_params(handle, hw_params)) < 0) {
            fprintf(stderr, "Cannot set hw params: %s\n", snd_strerror(retval));
            snd_pcm_close(capture_handle);
            snd_pcm_close(playback_handle);
            return 1;
        }

        snd_pcm_prepare(handle);
    }

}

int16_t buffer[PERIOD_SIZE * CHANNELS];

/* Loop for the sound processing*/
int Sound_Loop(){
    int retval;
    retval = snd_pcm_readi(capture_handle, buffer, PERIOD_SIZE); 
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
        // for (int i = 0; i < retval; i++) {
        //     process_sample(&buffer[i * CHANNELS], &buffer[i * CHANNELS]);
        // }

        retval = snd_pcm_writei(playback_handle, buffer, retval);
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



float prev_input = 0.0f;
float prev_output = 0.0f;
float alpha = 0.60f; // adjust cutoff frequency by alpha

float high_pass(float input) {
    float output = alpha * (prev_output + input - prev_input);
    prev_input = input;
    prev_output = output;
    return output;
}


// Example processing function (copy input to output)
void process_sample(int16_t *in, int16_t *out) {
    // printf("In: %d %d\n", in[0], in[1]);
    for (int i = 0; i < 2; i++){
        out[i] = in[i];
    }
    // printf("In: %6d %6d, Out: %6d, %6d\n", in[0], in[1], out[0], out[1]);
}



int play_sine() {
    snd_pcm_t *playback_handle;
    snd_pcm_hw_params_t *hw_params;
    int err;

    if ((err = snd_pcm_open(&playback_handle, PLAYBACK_DEVICE, SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
        fprintf(stderr, "Cannot open playback device: %s\n", snd_strerror(err));
        return 1;
    }

    snd_pcm_hw_params_alloca(&hw_params);
    snd_pcm_hw_params_any(playback_handle, hw_params);
    snd_pcm_hw_params_set_access(playback_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(playback_handle, hw_params, FORMAT);
    snd_pcm_hw_params_set_channels(playback_handle, hw_params, CHANNELS);
    snd_pcm_hw_params_set_rate(playback_handle, hw_params, SAMPLE_RATE, 0);

    snd_pcm_uframes_t period_size = PERIOD_SIZE;
    snd_pcm_hw_params_set_period_size_near(playback_handle, hw_params, &period_size, 0);

    if ((err = snd_pcm_hw_params(playback_handle, hw_params)) < 0) {
        fprintf(stderr, "Cannot set hw params: %s\n", snd_strerror(err));
        snd_pcm_close(playback_handle);
        return 1;
    }

    snd_pcm_prepare(playback_handle);

    int16_t buffer[PERIOD_SIZE * CHANNELS];
    double frequency = 1000.0;  // A4 note
    double phase = 0.0;
    double phase_increment = 2.0 * M_PI * frequency / SAMPLE_RATE;

    while (1) {
        for (int i = 0; i < PERIOD_SIZE; i++) {
            int16_t sample = (int16_t)(sin(phase) * 32767); // Max amplitude for 16-bit

            // Stereo: same sample for left and right
            buffer[2*i] = sample;       // Left channel
            buffer[2*i + 1] = sample;   // Right channel

            phase += phase_increment;
            if (phase >= 2.0 * M_PI) phase -= 2.0 * M_PI;
        }

        err = snd_pcm_writei(playback_handle, buffer, PERIOD_SIZE);
        if (err < 0) {
            if (err == -EPIPE) {
                fprintf(stderr, "Buffer underrun occurred\n");
                snd_pcm_prepare(playback_handle);
            } else {
                fprintf(stderr, "Playback retval: %s\n", snd_strerror(err));
                break;
            }
        } else if (err != PERIOD_SIZE) {
            fprintf(stderr, "Short write, wrote %d frames\n", err);
        }
    }

    snd_pcm_close(playback_handle);
    return 0;
}