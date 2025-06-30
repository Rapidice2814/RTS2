#include "audioProcessing.h"
#include <stdio.h>
#include "utils.h"


static audio_processing_args_t *audio_processing_args = NULL;

void* Function_Audio_Processing(void* arg) {
    audio_processing_args = (audio_processing_args_t *)arg;

    printf("FIFO in: %p, FIFO out: %p\n", 
           audio_processing_args->in_fifo, 
           audio_processing_args->out_fifo);

    int16_t in_sample;
    int16_t out_sample;

    while (1) {
        fifo_pop(audio_processing_args->in_fifo, &in_sample);
        out_sample = multiply_and_clip(in_sample, 20); // Process the sample (currently just a pass-through)
        fifo_push(audio_processing_args->out_fifo, &out_sample);
    }

    return NULL;
}
