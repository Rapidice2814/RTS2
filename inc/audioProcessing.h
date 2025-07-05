#pragma once
#include "fifo.h"

typedef struct {
    fifo_t *in_fifo;
    fifo_t *out_fifo;
    fifo_t *echo_fifo;
} audio_processing_args_t;

typedef struct {
    fifo_t *in_fifo;
    fifo_t *out_fifo;
} audio_simple_node_args_t;

void* Function_Audio_Echo_Cancelling(void* arg);
void* Function_Audio_Volume_Leveler(void* arg);