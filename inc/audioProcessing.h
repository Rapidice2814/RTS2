#pragma once
#include "fifo.h"

typedef struct {
    fifo_t *in_fifo;
    fifo_t *out_fifo;
    fifo_t *echo_fifo;
} audio_processing_args_t;

void* Function_Audio_Processing(void* arg);