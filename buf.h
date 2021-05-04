// Gregor McWilliam -- gbm5862

#ifndef BUF_H_
#define BUF_H_

#include <stdlib.h>
#include <sndfile.h>
#include <portaudio.h>
#include <stdatomic.h>
#include "paUtils.h"

#define MAX_ROUNDS          100
#define MAX_PATH_LEN        256
#define MAX_FILES           8
#define MAX_CHAN            2
#define FRAMES_PER_BUFFER   1024 
#define DEBUG               0

#define MAX_LEN     100

// Buf struct
typedef struct {
    atomic_int selection;
    unsigned int channels;
    unsigned int samplerate;
    float *x[MAX_FILES];
    unsigned long frames[MAX_FILES];
    unsigned long next_frame[MAX_FILES];
    unsigned long round;
    int gameArray[MAX_ROUNDS];
    int playback;
    int curRound;
    int readyForUserInput;
    int notification;
} Buf;

#endif /* BUF_H_ */