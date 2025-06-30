#pragma once

#include "fifo.h"


typedef struct {
    fifo_t *input_fifo;
    fifo_t *output_fifo;
} audio_io_args_t;

void* Function_AudioIO(void* arg);