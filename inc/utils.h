#pragma once
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

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
#define SAMPLE_RATE 48000  // Sample rate in Hz

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