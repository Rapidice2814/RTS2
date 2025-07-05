#include "audioProcessing.h"
#include <stdio.h>
#include "utils.h"
#include <string.h>

#include <speex/speex_echo.h>
#include <speex/speex_preprocess.h>

#include <time.h>

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
    float agc_level = 32766;  // Typical max: 32767 (max int16)

    speex_preprocess_ctl(preprocess_state, SPEEX_PREPROCESS_SET_DENOISE, &denoise);
    speex_preprocess_ctl(preprocess_state, SPEEX_PREPROCESS_SET_AGC, &agc);
    speex_preprocess_ctl(preprocess_state, SPEEX_PREPROCESS_SET_DEREVERB, &dereverb);
    speex_preprocess_ctl(preprocess_state, SPEEX_PREPROCESS_SET_AGC_LEVEL, &agc_level);

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
    int dereverb = 0;
    float agc_level = 30000;  // Typical max: 32767 (max int16)

    speex_preprocess_ctl(preprocess_state, SPEEX_PREPROCESS_SET_DENOISE, &denoise);
    speex_preprocess_ctl(preprocess_state, SPEEX_PREPROCESS_SET_AGC, &agc);
    speex_preprocess_ctl(preprocess_state, SPEEX_PREPROCESS_SET_DEREVERB, &dereverb);
    speex_preprocess_ctl(preprocess_state, SPEEX_PREPROCESS_SET_AGC_LEVEL, &agc_level);

}

void preprocess_deinit() {
    if (preprocess_state) {
        speex_preprocess_state_destroy(preprocess_state);
        preprocess_state = NULL;
    }
}







static struct timespec start_echo, end_echo;
static audio_processing_args_t *audio_processing_args = NULL;

void* Function_Audio_Echo_Cancelling(void* arg) {
    audio_processing_args = (audio_processing_args_t *)arg;

    printf("echo cancel: FIFO in: %p, FIFO out: %p\n", 
           audio_processing_args->in_fifo, 
           audio_processing_args->out_fifo);


    init_audio_processing();

    int16_t mic_frame[FRAME_SIZE];      // Captured microphone input
    int16_t echo_frame[FRAME_SIZE] = {0};     // Playback signal (reference)
    int16_t processed_frame[FRAME_SIZE];

    int16_t zero_buffer[FRAME_SIZE] = {0};  // initializes all to 0
    fifo_push_batch(audio_processing_args->echo_fifo, zero_buffer, FRAME_SIZE);


    while (1) {
        clock_gettime(CLOCK_MONOTONIC, &start_echo);
        
        fifo_pop_batch(audio_processing_args->in_fifo, mic_frame, FRAME_SIZE);
        if (fifo_try_pop_batch(audio_processing_args->echo_fifo, echo_frame, FRAME_SIZE) == 0) {
            // Not enough data, fill echo_frame with zeros
            memset(echo_frame, 0, FRAME_SIZE * sizeof(int16_t));
        }
        // for (int i = 0; i < FRAME_SIZE; i++) {
        //     mic_frame[i] = multiply_and_clip(mic_frame[i], 1);;
        // }



        if(1){
          speex_echo_cancellation(echo_state, mic_frame, echo_frame, processed_frame);  
        }else{
            for(int i = 0; i < FRAME_SIZE; i++) {
                processed_frame[i] = mic_frame[i]; //just copy mic
            }
        }


        // speex_preprocess_run(preprocess_state, processed_frame);

        // printf("Processed frame: %d\n", processed_frame[0]);

        // Push the processed frame
        fifo_push_batch(audio_processing_args->out_fifo, processed_frame, FRAME_SIZE);

        clock_gettime(CLOCK_MONOTONIC, &end_echo);
        double elapsed = ((end_echo.tv_sec - start_echo.tv_sec) + (end_echo.tv_nsec - start_echo.tv_nsec) / 1e9) * 1000000;
        printf("Echo time: %fus\n", elapsed);

    }

    destroy_audio_processing();

    return NULL;
}

static struct timespec start_volume, end_volume;
static audio_simple_node_args_t *audio_volume_leveler_args = NULL;
void* Function_Audio_Volume_Leveler(void* arg){
    audio_volume_leveler_args = (audio_simple_node_args_t *)arg;

    printf("audio leveler: FIFO in: %p, FIFO out: %p\n", 
           audio_volume_leveler_args->in_fifo, 
           audio_volume_leveler_args->out_fifo);

    int16_t frame[FRAME_SIZE];

    while (1) {
        clock_gettime(CLOCK_MONOTONIC, &start_volume);

        fifo_pop_batch(audio_volume_leveler_args->in_fifo, frame, FRAME_SIZE);
        

        speex_preprocess_run(preprocess_state, frame);
        // Apply volume leveler (simple example)
        // for (int i = 0; i < FRAME_SIZE; i++) {
        //     frame[i] = multiply_and_clip(frame[i], 1); // Adjust volume factor as needed
        // }

        fifo_push_batch(audio_volume_leveler_args->out_fifo, frame, FRAME_SIZE);

        clock_gettime(CLOCK_MONOTONIC, &end_volume);
        double elapsed = ((end_volume.tv_sec - start_volume.tv_sec) + (end_volume.tv_nsec - start_volume.tv_nsec) / 1e9) * 1000000;
        printf("Vol time: %fus\n", elapsed);
    }

    return NULL;

}