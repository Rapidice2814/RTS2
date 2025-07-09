#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <ncurses.h>
#include <stdbool.h>
#include <unistd.h>  // for sleep
#include <string.h>
#include <stdarg.h>
#include "UI.h"

#define MAX_LOG_LINES 10
#define MAX_LOG_LINE_LEN 128

#define VOLUME_BAR_WIDTH 20

static char log_lines[MAX_LOG_LINES][MAX_LOG_LINE_LEN];
static int log_line_index = 0;
static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

void log_message(const char* fmt, ...) {
    pthread_mutex_lock(&log_mutex);

    va_list args;
    va_start(args, fmt);
    vsnprintf(log_lines[log_line_index], MAX_LOG_LINE_LEN, fmt, args);
    va_end(args);

    log_line_index = (log_line_index + 1) % MAX_LOG_LINES;

    pthread_mutex_unlock(&log_mutex);
}

static struct timespec start_ui, end_ui;

void* ui_thread_func(void* arg) {
    UIState* state = (UIState*)arg;

    initscr();
    noecho();
    cbreak();
    nodelay(stdscr, TRUE);  // Non-blocking input
    keypad(stdscr, TRUE);

    while (1) {
        clock_gettime(CLOCK_MONOTONIC, &start_ui);
        pthread_mutex_lock(&state->lock);
        bool denoise = state->denoise;
        bool agc = state->agc;
        bool dereverb = state->dereverb;
        bool echo = state->echo;
        float volume = state->volume;
        pthread_mutex_unlock(&state->lock);

        int volume_bar = (int)(volume * VOLUME_BAR_WIDTH);

        clear();
        printw("Echo Canceller UI\n");
        printw("-----------------\n");
        printw("d - Toggle Denoise     : %s\n", denoise ? "ON" : "OFF");
        printw("a - Toggle AGC         : %s\n", agc ? "ON" : "OFF");
        printw("r - Toggle Dereverb    : %s\n", dereverb ? "ON" : "OFF");
        printw("e - Toggle Echo Cancel  : %s\n", echo ? "ON" : "OFF");
        printw("v - Volume (%.0f%%): [", volume * 100);
        for (int i = 0; i < VOLUME_BAR_WIDTH; ++i) {
            if (i < volume_bar)
                printw("#");
            else
                printw(" ");
        }
        printw("]\n");
        printw("\nPress keys to toggle options.\n");

        // Display logs at bottom
        int term_rows, term_cols;
        getmaxyx(stdscr, term_rows, term_cols);

        pthread_mutex_lock(&log_mutex);
        int log_start_row = term_rows - MAX_LOG_LINES - 1;
        if (log_start_row > 0) {
            mvprintw(log_start_row++, 0, "Logs:");
            for (int i = 0; i < MAX_LOG_LINES; ++i) {
                int idx = (log_line_index + i) % MAX_LOG_LINES;
                mvprintw(log_start_row++, 0, "%s", log_lines[idx]);
            }
        }
        pthread_mutex_unlock(&log_mutex);

        refresh();

        int ch = getch();
        if (ch != ERR) {
            pthread_mutex_lock(&state->lock);
            switch (ch) {
                case 'd': state->denoise = !state->denoise; break;
                case 'a': state->agc = !state->agc; break;
                case 'r': state->dereverb = !state->dereverb; break;
                case 'e': state->echo = !state->echo; break;
                case 'v':  // Optional reset key
                    state->volume = 0.5f;
                    break;
                case KEY_UP:
                case '+':
                    if (state->volume < 1.0f)
                        state->volume += 0.05f;
                    if (state->volume > 1.0f)
                        state->volume = 1.0f;
                    break;
                case KEY_DOWN:
                case '-':
                    if (state->volume > 0.0f)
                        state->volume -= 0.05f;
                    if (state->volume < 0.0f)
                        state->volume = 0.0f;
                    break;
            }
            pthread_mutex_unlock(&state->lock);
        }
        clock_gettime(CLOCK_MONOTONIC, &end_ui);
        double elapsed = ((end_ui.tv_sec - start_ui.tv_sec) + (end_ui.tv_nsec - start_ui.tv_nsec) / 1e9) * 1000000;
        static double max_elapsed = 0;
        if (elapsed > max_elapsed && elapsed < 100000) {
            max_elapsed = elapsed;
        }
        // log_message("U Time: %.2fus (Max: %.2fus)", elapsed, max_elapsed);

        usleep(100 * 1000);  // 100 ms
    }

    endwin();
    return NULL;
}
