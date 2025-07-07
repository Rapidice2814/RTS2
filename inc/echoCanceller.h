#pragma once

#include <stdlib.h>
#include <stdint.h>


typedef struct MyEchoState MyEchoState;

// Create echo canceller with configurable frame size and echo tail (filter length)
MyEchoState* my_echo_state_init(int frame_size, int filter_length);

// Destroy echo canceller
void my_echo_state_destroy(MyEchoState* st);

// Perform echo cancellation
void my_echo_cancellation(MyEchoState* st, const int16_t* mic_frame, const int16_t* echo_frame, int16_t* out_frame);

