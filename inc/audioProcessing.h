#pragma once
#include "fifo.h"
#include "UI.h"

typedef struct {
    fifo_t *in_fifo;
    fifo_t *out_fifo;
    fifo_t *echo_fifo;
    UIState *ui_state;  
} audio_processing_args_t;

typedef struct {
    fifo_t *in_fifo;
    fifo_t *out_fifo;
    UIState *ui_state;  
} audio_simple_node_args_t;

void* Function_Audio_Echo_Cancelling(void* arg);
void* Function_Audio_Volume_Leveler(void* arg);
void* AudioSettings(void* arg);