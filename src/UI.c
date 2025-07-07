#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <ncurses.h>
#include <stdbool.h>
#include <unistd.h>  // for sleep
#include "UI.h"


void* ui_thread_func(void* arg) {
    UIState* state = (UIState*)arg;

    initscr();
    noecho();
    cbreak();
    nodelay(stdscr, TRUE);  // Non-blocking input
    keypad(stdscr, TRUE);

    while (1) {
        pthread_mutex_lock(&state->lock);
        bool denoise = state->denoise;
        bool agc = state->agc;
        bool dereverb = state->dereverb;
        bool echo = state->echo;
        pthread_mutex_unlock(&state->lock);

        // if (!running) break;

        clear();
        printw("Echo Canceller UI\n");
        printw("-----------------\n");
        printw("d - Toggle Denoise     : %s\n", denoise ? "ON" : "OFF");
        printw("a - Toggle AGC         : %s\n", agc ? "ON" : "OFF");
        printw("r - Toggle Dereverb    : %s\n", dereverb ? "ON" : "OFF");
        printw("e - Toggle Echo Canell : %s\n", echo ? "ON" : "OFF");
        printw("\nPress keys to toggle options.\n");
        refresh();

        int ch = getch();
        if (ch != ERR) {
            pthread_mutex_lock(&state->lock);
            switch (ch) {
                case 'd': state->denoise = !state->denoise; break;
                case 'a': state->agc = !state->agc; break;
                case 'r': state->dereverb = !state->dereverb; break;
                case 'e': state->echo = !state->echo; break;
            }
            pthread_mutex_unlock(&state->lock);
        }

        usleep(100 * 1000);  // 100 ms
    }

    endwin();
    return NULL;
}