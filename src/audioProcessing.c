#include "audioProcessing.h"
#include <stdio.h>
#include "utils.h"

#include <speex/speex_echo.h>
#include <speex/speex_preprocess.h>

#define FRAME_SIZE (48 * 2)
#define SAMPLE_RATE 48000
#define ECHO_TAIL_MS 100

SpeexEchoState *echo_state = NULL;
static SpeexPreprocessState *preprocess_state = NULL;


void init_audio_processing() {
    int sample_rate = SAMPLE_RATE;
    int echo_tail = (SAMPLE_RATE * ECHO_TAIL_MS) / 1000; // e.g., 1600 samples for 100ms tail

    // Create echo canceller
    echo_state = speex_echo_state_init(FRAME_SIZE, echo_tail);
    if (!echo_state) {
        fprintf(stderr, "Failed to initialize echo canceller\n");
        return;
    }
    speex_echo_ctl(echo_state, SPEEX_ECHO_SET_SAMPLING_RATE, &sample_rate);

    // Create preprocessor
    preprocess_state = speex_preprocess_state_init(FRAME_SIZE, sample_rate);
    if (!preprocess_state) {
        fprintf(stderr, "Failed to initialize preprocessor\n");
        speex_echo_state_destroy(echo_state);
        echo_state = NULL;
        return;
    }

    // Connect echo canceller to preprocessor
    speex_preprocess_ctl(preprocess_state, SPEEX_PREPROCESS_SET_ECHO_STATE, echo_state);

    // Enable options
    int denoise = 1;
    int agc = 1;
    int dereverb = 0;
    int agc_level = 30000; // target RMS level (0–32767, Q15 format)

    speex_preprocess_ctl(preprocess_state, SPEEX_PREPROCESS_SET_DENOISE, &denoise);
    speex_preprocess_ctl(preprocess_state, SPEEX_PREPROCESS_SET_AGC, &agc);
    speex_preprocess_ctl(preprocess_state, SPEEX_PREPROCESS_SET_AGC_LEVEL, &agc_level);
    speex_preprocess_ctl(preprocess_state, SPEEX_PREPROCESS_SET_DEREVERB, &dereverb);

    printf("Audio processing (AEC + preprocessor) initialized.\n");
}

void destroy_audio_processing() {
    if (preprocess_state) {
        speex_preprocess_state_destroy(preprocess_state);
        preprocess_state = NULL;
    }
    if (echo_state) {
        speex_echo_state_destroy(echo_state);
        echo_state = NULL;
    }
}


void preprocess_init() {
    preprocess_state = speex_preprocess_state_init(FRAME_SIZE, SAMPLE_RATE);

    int denoise = 1;
    int agc = 1;
    int dereverb = 1;

    speex_preprocess_ctl(preprocess_state, SPEEX_PREPROCESS_SET_DENOISE, &denoise);
    speex_preprocess_ctl(preprocess_state, SPEEX_PREPROCESS_SET_AGC, &agc);
    speex_preprocess_ctl(preprocess_state, SPEEX_PREPROCESS_SET_DEREVERB, &dereverb);

    float agc_level = 30000;  // Typical max: 32767 (max int16)
    speex_preprocess_ctl(preprocess_state, SPEEX_PREPROCESS_SET_AGC_LEVEL, &agc_level);
}

void preprocess_deinit() {
    if (preprocess_state) {
        speex_preprocess_state_destroy(preprocess_state);
        preprocess_state = NULL;
    }
}








static audio_processing_args_t *audio_processing_args = NULL;

void* Function_Audio_Processing(void* arg) {
    audio_processing_args = (audio_processing_args_t *)arg;

    printf("FIFO in: %p, FIFO out: %p\n", 
           audio_processing_args->in_fifo, 
           audio_processing_args->out_fifo);

    // int16_t in_sample;
    // int16_t out_sample;

    preprocess_init();

    int16_t in_buffer[FRAME_SIZE];
    int16_t out_buffer[FRAME_SIZE];

    while (1) {
        // fifo_pop(audio_processing_args->in_fifo, &in_sample);
        // out_sample = multiply_and_clip(in_sample, 10); // multiply input by 10
        // fifo_push(audio_processing_args->out_fifo, &out_sample);
        // Pop FRAME_SIZE samples from input FIFO (blocks until available)

        fifo_pop_batch(audio_processing_args->in_fifo, in_buffer, FRAME_SIZE);

        // SpeexDSP expects in_buffer in-place
        // Process with SpeexDSP preprocess (denoise + AGC + dereverb)
        speex_preprocess_run(preprocess_state, in_buffer);

        // Copy processed frame to out_buffer (could be in_buffer itself)
        for (int i = 0; i < FRAME_SIZE; i++) {
            out_buffer[i] = in_buffer[i];//multiply_and_clip(in_buffer[i], 10);;
        }

        // Push processed batch to output FIFO
        fifo_push_batch(audio_processing_args->out_fifo, out_buffer, FRAME_SIZE);


    }

    preprocess_deinit();

    return NULL;
}

// void* Function_Audio_Processing(void* arg) {
//     audio_processing_args = (audio_processing_args_t *)arg;

//     printf("FIFO in: %p, FIFO out: %p\n", 
//            audio_processing_args->in_fifo, 
//            audio_processing_args->out_fifo);


//     init_audio_processing();

//     int16_t mic_frame[FRAME_SIZE];      // Captured microphone input
//     int16_t echo_frame[FRAME_SIZE];     // Playback signal (reference)
//     int16_t processed_frame[FRAME_SIZE];

//     while (1) {
//         // Pop input (mic) and playback (echo ref)
//         fifo_pop_batch(audio_processing_args->in_fifo, mic_frame, FRAME_SIZE);
//         // fifo_pop_batch(audio_processing_args->echo_fifo, echo_frame, FRAME_SIZE);  // New FIFO!

//         // Perform echo cancellation
//         // speex_echo_cancellation(echo_state, mic_frame, echo_frame, processed_frame);

//         // Run additional processing (AGC, denoise, etc.)
//         for(int i = 0; i < FRAME_SIZE; i++) {
//             processed_frame[i] = mic_frame[i]; // For now, just copy mic input
//         }

//         speex_preprocess_run(preprocess_state, processed_frame);

//         // Push the processed frame
//         fifo_push_batch(audio_processing_args->out_fifo, processed_frame, FRAME_SIZE);
//     }

//     destroy_audio_processing();

//     return NULL;
// }
