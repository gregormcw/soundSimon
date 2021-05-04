// Gregor McWilliam -- gbm5862

#include <stdlib.h>
#include <string.h>
#include <ncurses.h>
#include <time.h>
#include <sndfile.h>
#include <portaudio.h>
#include <stdatomic.h>
#include "game.h"
#include "buf.h"
#include "paUtils.h"


// Prints start screen to console
void printStartScreen(void) {
    printw("==========================================\n");
    printw("         Welcome to Sound Simon!\n");
    printw("==========================================\n");
    printw("==========================================\n");
    printw(" Controls: a = left, w = front, d = right\n");
    printw("==========================================\n");
    printw("       ** Hit spacebar to start **\n");
    printw("          ** Press p to quit **\n");
    printw("\n");
}


// Plays current audio sequence to user, pausing between each index
void printGameState(Game *gp, Buf *p, PaStream *stream) {
    
    printw("\t\t ROUND %d\n ", gp->round+1);

    if (DEBUG) {
        for (int i = 0; i < gp->round+1; i++) {
            printw("\t       Round %d: %c\n", i+1, gp->gameArray[i]);
        }
    }

    printw("       Can you repeat what I played?\n\n");
}


// Determines whether user input is valid
int isValid(int c) {

    int validKeys[6] = {'a', 'w', 'd', 'D', 'A', 'C'};

    for (int i = 0; i < 6; i++) {
        if (c == validKeys[i])
            return 1;
    }
    return 0;
}


// Generates random number in range [0, 2]
int getRandNum(void) {

    int randNum;
    int upper = 2;
    int lower = 0;

    randNum = rand() % (1 + upper - lower);
    return randNum;
}


// Converts user input to relevant output channel value
int intToIdx(int c) {

    if (c == 'a')
        return 0;

    if (c == 'w')
        return 1;

    if (c == 'd')
        return 2;

    return -1;
}


// Assigns random number to game array at current index
void newGameArrayVal(Game *gp, Buf *p, PaStream *stream) {

    int randNum = getRandNum();
    int numKey[3] = {'a', 'w', 'd'};

    gp->gameArray[gp->round] = numKey[randNum];
    p->gameArray[gp->round] = numKey[randNum];
    p->curRound = 0;
    p->selection = randNum;
    p->readyForUserInput = 0;
    p->playback = 1;
}
