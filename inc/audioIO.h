#pragma once

#include "fifo.h"


typedef struct {
    fifo_t *capture_fifo;
    fifo_t *playback_fifo;
} audio_io_args_t;

void* Function_AudioIO(void* arg);