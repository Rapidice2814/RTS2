#include "echoCanceller.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define EPSILON 1e-8f
#define MU 0.1f

struct MyEchoState {
    int frame_size;
    int filter_length;

    float* filter;
    float* echo_buffer;
};

MyEchoState* my_echo_state_init(int frame_size, int filter_length) {
    if (frame_size <= 0 || filter_length <= 0) return NULL;

    MyEchoState* st = (MyEchoState*)malloc(sizeof(MyEchoState));
    if (!st) return NULL;

    st->frame_size = frame_size;
    st->filter_length = filter_length;
    st->filter = (float*)calloc(filter_length, sizeof(float));
    st->echo_buffer = (float*)calloc(filter_length, sizeof(float));

    if (!st->filter || !st->echo_buffer) {
        my_echo_state_destroy(st);
        return NULL;
    }

    return st;
}

void my_echo_state_destroy(MyEchoState* st) {
    if (!st) return;
    free(st->filter);
    free(st->echo_buffer);
    free(st);
}

void my_echo_cancellation(MyEchoState* st, const int16_t* mic_frame, const int16_t* echo_frame, int16_t* out_frame) {
    if (!st || !mic_frame || !echo_frame || !out_frame) return;

    for (int i = 0; i < st->frame_size; ++i) {
        // Shift echo buffer
        memmove(&st->echo_buffer[1], &st->echo_buffer[0], (st->filter_length - 1) * sizeof(float));
        st->echo_buffer[0] = (float)echo_frame[i];

        // Predict echo
        float echo_estimate = 0.0f;
        for (int j = 0; j < st->filter_length; ++j) {
            echo_estimate += st->filter[j] * st->echo_buffer[j];
        }

        float mic_input = (float)mic_frame[i];
        float error = mic_input - echo_estimate;

        // Clamp output
        float cleaned = error;
        if (cleaned > 32767.0f) cleaned = 32767.0f;
        if (cleaned < -32768.0f) cleaned = -32768.0f;
        out_frame[i] = (int16_t)cleaned;

        // Update filter
        float power = EPSILON;
        for (int j = 0; j < st->filter_length; ++j) {
            power += st->echo_buffer[j] * st->echo_buffer[j];
        }
        for (int j = 0; j < st->filter_length; ++j) {
            st->filter[j] += (MU * error * st->echo_buffer[j]) / power;
        }
    }
}
