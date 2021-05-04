// Gregor McWilliam -- gbm5862

#ifndef GAME_H_
#define GAME_H_

#include "buf.h"

#define MAX_LEN     100

// Game struct
typedef struct {
	int gameArray[MAX_LEN];
	int round;
	int isPlaying;
	int lastInputValid;
} Game;

    // Declare function prototypes
    int isValid(int c);
    void printStartScreen(void);
    void printGameState(Game *gp, Buf *p, PaStream *stream);
    int getRandNum(void);
    void newGameArrayVal(Game *gp, Buf *p, PaStream *stream);
    int intToIdx(int c);

#endif /* GAME_H_ */