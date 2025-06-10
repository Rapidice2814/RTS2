#include "audio.h"

#include <math.h> 
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <alsa/asoundlib.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif


#define DEVICE "hw:3"
#define SAMPLE_RATE 48000
#define CHANNELS 2
#define FORMAT SND_PCM_FORMAT_S16_LE

#define PERIOD_SIZE 128
// #define PERIOD_SIZE 10000

// Example processing function (copy input to output)
void process_sample(int16_t *in, int16_t *out) {
    printf("In: %d %d\n", in[0], in[1]);
    out[0] = in[0]; // Left
    out[1] = in[1]; // Right

    // out[0] = 0;
    // out[1] = 0;
}

int play_sine();

int sound_processing() {
    // play_sine();
    snd_pcm_t *capture_handle, *playback_handle;
    snd_pcm_hw_params_t *hw_params;
    int err;

    // Open capture device
    if ((err = snd_pcm_open(&capture_handle, DEVICE, SND_PCM_STREAM_CAPTURE, 0)) < 0) {
        fprintf(stderr, "Cannot open capture device: %s\n", snd_strerror(err));
        return 1;
    }

    // Open playback device
    if ((err = snd_pcm_open(&playback_handle, DEVICE, SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
        fprintf(stderr, "Cannot open playback device: %s\n", snd_strerror(err));
        snd_pcm_close(capture_handle);
        return 1;
    }

    // Configure both devices
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

        if ((err = snd_pcm_hw_params(handle, hw_params)) < 0) {
            fprintf(stderr, "Cannot set hw params: %s\n", snd_strerror(err));
            snd_pcm_close(capture_handle);
            snd_pcm_close(playback_handle);
            return 1;
        }

        snd_pcm_prepare(handle);
    }

    // Buffer for PERIOD_SIZE frames, 2 channels per frame
    int16_t buffer[PERIOD_SIZE * CHANNELS];

    while (1) {
        err = snd_pcm_readi(capture_handle, buffer, PERIOD_SIZE);
        if (err < 0) {
            if (err == -EPIPE) {
                snd_pcm_prepare(capture_handle);
                continue;
            } else {
                fprintf(stderr, "Capture error: %s\n", snd_strerror(err));
                break;
            }
        } else if (err != PERIOD_SIZE) {
            fprintf(stderr, "Short read, expected %d frames, got %d\n", PERIOD_SIZE, err);
        }

        // Process each sample pair (frame) individually
        for (int i = 0; i < err; i++) {
            process_sample(&buffer[i * CHANNELS], &buffer[i * CHANNELS]);
        }

        err = snd_pcm_writei(playback_handle, buffer, PERIOD_SIZE);
        if (err < 0) {
            if (err == -EPIPE) {
                snd_pcm_prepare(playback_handle);
            } else {
                fprintf(stderr, "Playback error: %s\n", snd_strerror(err));
                break;
            }
        } else if (err != PERIOD_SIZE) {
            fprintf(stderr, "Short write, expected %d frames, got %d\n", PERIOD_SIZE, err);
        }
    }

    snd_pcm_close(capture_handle);
    snd_pcm_close(playback_handle);
    return 0;
}


int play_sine() {
    snd_pcm_t *playback_handle;
    snd_pcm_hw_params_t *hw_params;
    int err;

    if ((err = snd_pcm_open(&playback_handle, DEVICE, SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
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
                fprintf(stderr, "Playback error: %s\n", snd_strerror(err));
                break;
            }
        } else if (err != PERIOD_SIZE) {
            fprintf(stderr, "Short write, wrote %d frames\n", err);
        }
    }

    snd_pcm_close(playback_handle);
    return 0;
}