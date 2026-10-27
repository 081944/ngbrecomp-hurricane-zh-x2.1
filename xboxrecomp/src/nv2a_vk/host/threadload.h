/* How much of each second the program's threads use, for log.txt: which thread runs out of time on a
 * slow CPU. A thread calls recomp_load_sample with its slot now and then (its CPU time is read there,
 * on the thread itself); the title's thread calls recomp_load_report once a frame, and every five
 * seconds that prints a line like
 *   [LOAD] game 184 ms/s, GPU model 21, audio 7, all threads 265, of 1600 on 16 CPUs
 */
#pragma once

enum {
    RECOMP_LOAD_GAME = 0,   /* the title's thread (the one that presents) */
    RECOMP_LOAD_GPU,        /* the NV2A model's push-buffer thread */
    RECOMP_LOAD_AUDIO,      /* the sound output's callback */
    RECOMP_LOAD_SLOTS
};

void recomp_load_sample(int slot);
void recomp_load_report(void);
