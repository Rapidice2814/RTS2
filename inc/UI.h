#pragma once

#include <stdbool.h>

typedef struct {
    bool denoise;
    bool agc;
    bool dereverb;
    bool echo;
    pthread_mutex_t lock;
} UIState;

void* ui_thread_func(void* arg);