#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ncurses.h>
#include <time.h>
#include "unistd.h"
#include "game.h"
#include <sndfile.h>
#include <portaudio.h>
#include "paUtils.h"
#include <stdatomic.h>

// Maximum length of game array
// (Maximum number of rounds)
#define MAX_LEN     100

#define MAX_PATH_LEN        256
#define MAX_FILES           8
#define MAX_CHAN            2
#define FRAMES_PER_BUFFER   1024 
#define NUM_OPTIONS         3

#define DEBUG               0


/*  TO DO - 04/25:

-   Play audio once for each sequence index (stop pA looping)
-   Play audio once for each user input
-   Clean up
-   Additional SFX
-   Ensure use of invalid multi-char keys, e.g. arrow keys, don't end game

*/

// Make two functions:
// playSound(float *x, int framesToPlay, int channel);
//      make stereo file
//      Start up portAudio
//      play until end of buffer
//      Shut down portAudio

// Struct for audio
// x buffer, number of frames in x buffer
// output channels

typedef struct {
    atomic_int selection;   // Start, error, end, move
    unsigned int channels;
    unsigned int samplerate;
    float *x[MAX_FILES];
    unsigned long frames[MAX_FILES];
    unsigned long next_frame[MAX_FILES];
    unsigned long round;
    int gameArray[MAX_LEN];
    int playback;
    int curRound;
    // Start tone, first move, second move
    // 0 move = start?
    // Specific buffer for zeros
    // Flag somewhere in callback? -1 for zeros
    // 0 for start tone
    // play through all rounds

} Buf;

static int paCallback( const void *inputBuffer, void *outputBuffer,
    unsigned long framesPerBuffer,
    const PaStreamCallbackTimeInfo* timeInfo,
    PaStreamCallbackFlags statusFlags,
    void *userData );


// MAIN(): START
///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////


int main(int argc, char** argv) {

    Game game;
    Game *gp = &game;

    FILE *fp;
    Buf buf, *p = &buf;
    SNDFILE *sndfile;
    SF_INFO sfinfo;
    PaStream *stream;

    char ifilename[MAX_FILES][MAX_PATH_LEN];
    int i;
    float count;
    unsigned int num_input_files;

    memset(&sfinfo, 0, sizeof(sfinfo));

    // If number of command-line arguments is not 2, print usage statement
    if (argc != 2) {
        fprintf(stderr, "Usage: %s *.txt\n", argv[0]);
        return -1;
    }

    /* Open list of files */

    // Direct file pointer to command line argument
    fp = fopen(argv[1], "r");

    // If fp points to null value, print error statement and return -1
    if (!fp) {
        fprintf(stderr, "Cannot open %s\n", argv[1]);
        return -1;
    }

    /* read WAV filenames from the list in ifile_list.txt */
    /* print information about each WAV file */


// AUDIO FILE READ: START
///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////

    // Read in all sound effects and assign to SNDFILE struct

    // Iterate through all files
    for (i=0; i<MAX_FILES; i++) {

        // If end of ifilename array is reached, break out of loop
        if (fscanf(fp, "%s", ifilename[i]) == EOF)
            break;

        // Open ith element of ifilename with sndfile function
        sndfile = sf_open(ifilename[i], SFM_READ, &sfinfo);

        // If no valid information, return error statement and return -1
        if (!sndfile) {
            fprintf(stderr, "Cannot open %s\n", ifilename[i]);
            return -1;
        }

        // If debug option is selected, print header information for user
        if (DEBUG) {
            printf("%d Frames: %lld, Channels: %d, Samplerate: %d, %s\n", \
                i, sfinfo.frames, sfinfo.channels, sfinfo.samplerate, ifilename[i]);
        }

        /* check compatibility of input WAV files
         * If sample rates don't match, print error and exit 
         * If number of channels don't match, print error and exit
         * if too many channels, print error and exit
         */

        // Set number of input files to 0
        num_input_files = 0;
        
        // Retrieve frame information and assign to Buf structure
        p->frames[i] = sfinfo.frames;

        // Set next frame to 0
        p->next_frame[i] = 0;

        p->playback = 0;

        // On the first iteration, retrieve channel and sample rate information
        if (i == 0) {
            p->channels = sfinfo.channels;
            p->samplerate = sfinfo.samplerate;
        }

        // If number of channels beyond pre-determined max, print error statement
        // and return -1
        if (sfinfo.channels > MAX_CHAN) {
            fprintf(stderr, "Error: %s contains too many channels\n", ifilename[i]);
            return -1;
        }

        // If any files have a channel count inconsistent with the first file,
        // print error statement and return -1
        if (i > 0 && sfinfo.channels != p->channels) {
            fprintf(stderr, "Error: Inconsistent channel counts\n");
            return -1;
        }

        // If any files have a sample rate inconsistent with the first file,
        // print error statement and return -1
        if (i > 0 && sfinfo.samplerate != p->samplerate) {
            fprintf(stderr, "Error: Inconsistent sample rates\n");
            return -1;
        }

        // Allocate storage and read audio data into buffer
        p->x[i] = (float *)malloc(p->frames[i] * p->channels * sizeof(float));

        // If no valid audio data, print error statement and return -1
        if (!p->x[i]) {
            fprintf(stderr, "Error: Unable to read audio file\n");
            return -1;
        }

        // Extract frame information and assign to variable count
        count = sf_readf_float(sndfile, p->x[i], p->frames[i]);

        // If count is inconsistent with header-derived frame number, print
        // error statement and return -1
        if (count != p->frames[i]) {
            fprintf(stderr, "Error: Inconsistent audio frame count\n");
            return -1;
        }

        // Close WAV file
        sf_close(sndfile);
    }

    // Assign number of files iterated through to variable
    num_input_files = i;

    /* close input filelist */
    fclose(fp);

// AUDIO FILE READ: END
///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////


// GAME ENGINE: START
///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////


    // int round = 0; // SHOULD BE PART OF Game struct ************************
    // float fl = 0.1;
    srand(time(0));

    int isValid(int c);
    void printStartScreen(void);
    void printGameState(Game *gp, Buf *p, PaStream *stream);
    int getRandNum(void);
    void newGameArrayVal(Game *gp, Buf *p, PaStream *stream);
    int intToIdx(int c);
    void playSound(float *x, Buf *p, Game *gp, int framesToPlay, int selection);

    gp->round = 0;
    p->round = 0;
    gp->isPlaying = 1;
    gp->lastInputValid = 1;
    p->selection = -1;

    stream = startupPa(1, p->channels, p->samplerate, FRAMES_PER_BUFFER, 
        paCallback, &buf);

    // Initializes screen, sets up memory, and clears screen
    initscr();
    cbreak();
    noecho();

    // Prints string(const char*) to a window
    printStartScreen();

    // Refreshes screen to match what is in memory
    refresh();

    int c = getch();

    while (c != ' ' && c != 'p') {

        clear();
        printStartScreen();
        printw("%c is not a valid key.\n", c);
        c = getch();

    }

    clear();
    printStartScreen();

    if (c == ' ') {

        printw("         Starting now. Get ready!\n");
        // p->selection = 3;
        // Pa_Sleep(2000);

        // playGame(); // CREATE FUNCTION FROM WHICH GAME IS RUN ********************

        // Wait for user input, return int value of key
        while (gp->isPlaying) {

            if (gp->round > 0) {
                clear();
                printStartScreen();
                printw("Congratulations! You made it through round %d\n", gp->round);
                // p->selection = 4;
            }

            // Generate new array value
            if (gp->lastInputValid)
                newGameArrayVal(gp, p, stream);

            // Play all array values to user
            printGameState(gp, p, stream);
            // Pa_Sleep( (((gp->round+1) * p->frames[0]) / p->samplerate) * 1000);

            for (int i = 0; i < gp->round+1; i++) {

                c = getch();
                // p->selection = intToIdx(c);

                if (c == 'p') {
                    clear();
                    printStartScreen();
                    printw("\t  Thank you for playing!\n");
                    // napms(1000); // FIX THIS ********************
                    gp->isPlaying = 0;
                    // shutdownPa(stream); // SHOULD ONLY BE STATED AT END OF WHILE LOOP **
                    break;
                }

                while (!isValid(c)) {
                    gp->lastInputValid = 0;
                    clear();
                    printStartScreen();
                    printw("\t  %c is not a valid key\n", c);
                    c = getch();
                }

                gp->lastInputValid = 1;

                if (c != gp->gameArray[i]) {
                    gp->isPlaying = 0;
                    printw("\n          Wrong input. Game over!\n");
                    // p->selection = 5;
                    break;
                    napms(1000); // FIX THIS ********************

                }

                else if (c == gp->gameArray[i]) {
                    printw("          Correct! You entered: %c\n", c);

                    if (i == gp->round)
                        napms(1000); // FIX THIS *********************
                }             
            }
            gp->round += 1;
            p->round = gp->round;
        }
        printw("           Hit any key to exit\n"); // FIX THIS **********
        c = getch();
    }

    // Deallocate memory and close ncurses
    endwin();

    // Shut down PortAudio
    Pa_Sleep(500);
    shutdownPa(stream);

    return 0;
}


// MAIN(), GAME ENGINE: END
///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////


// HELPER FUNCTIONS: START
///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////


// Prints start screen to console
void printStartScreen(void) {
    printw("==========================================\n");
    printw("         Welcome to Sound Simon!\n");
    printw("==========================================\n");
    printw("==========================================\n");
    printw(" Controls: a = left, w = front, d = right\n");
    printw("==========================================\n");
    printw("       ** Hit SpaceBar to start **\n");
    printw("          ** Press p to quit **\n");
    printw("\n");
}


// Plays current audio sequence to user, pausing between each index
void printGameState(Game *gp, Buf *p, PaStream *stream) {
    
    printw("\t\t ROUND %d\n ", gp->round+1);

    for (int i = 0; i < gp->round+1; i++) {

        printw("\t       Round %d: %c\n", i+1, gp->gameArray[i]);

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
    sleep(1);
    p->curRound = 0;
    p->selection = randNum;
    p->playback = 1;
}


void playSound(float *x, Buf *p, Game *gp, int framesToPlay, int selection) {

    //      Make stereo file
    //      Start up portAudio
    //      Play until end of buffer
    //      Shut down portAudio

}


// AUDIO CALLBACK: START
///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////


// Processes audio output
static int paCallback(const void *inputBuffer, void *outputBuffer,
    unsigned long framesPerBuffer,
    const PaStreamCallbackTimeInfo* timeInfo,
    PaStreamCallbackFlags statusFlags,
    void *userData)
{

    // Initialize pointer to Buf
    Buf *p = (Buf *)userData; // Cast data passed through stream to our structure
    float *output = (float *)outputBuffer;
    atomic_int selection;
 
    // ===============================================================================
    // ===============================================================================
    // ===============================================================================
    // SELECTION SHOULD BE THE GAME ARRAY!
    // ===============================================================================
    // If selection is -1, all zeros
    // If selection < NUM_OPTIONS, then look for next selection round times then selection = -1
    // If selection > NUM_OPTIONS, play once then selection = -1
    // ===============================================================================
    // ===============================================================================
    // Selection now determines channel, and length of audio file is great.
    //      -> All that needs to happen is consistent stream from round 0 to current round, then sel = -1
    // ===============================================================================
    // ===============================================================================
    // ===============================================================================

    // Pointer to signal buffer, how many frames it is, where it is as you play it out
    // 

    //
    // PASS OUTPUT CHANNELS AS ARRAY *******************************
    //

    // CHANGE THIS

    if (!p->playback) {
        for (int i = 0; i < framesPerBuffer * p->channels; i++) {
            output[i] = 0.0;
        }
    }

    else {

        selection = intToIdx(p->gameArray[p->curRound]);

        // selection = p->gameArray[0];
        int next_sample, k = 0;
        int endOfBuff = p->next_frame[selection] + framesPerBuffer;
        unsigned long endOfSeq = (p->round+1) * p->frames[0];

        unsigned long seqIndex = p->curRound * p->frames[0] + p->next_frame[selection] + framesPerBuffer;

        if (seqIndex >= endOfSeq) {
            p->next_frame[selection] = 0;
            p->playback = 0;
        }

        if (endOfBuff >= p->frames[selection] && endOfBuff < endOfSeq) {
            p->next_frame[selection] = 0;
            p->selection = intToIdx(p->gameArray[p->curRound + 1]);
            p->curRound++;
            //printw("Now playing: %d\n", p->gameArray[++i]);
        }

        // Otherwise, set next sample as next frame * number of channels
        next_sample = p->next_frame[selection] * p->channels;

        // Iterate through the current buffer, considering all channels
        // Output channel determined by outputChannel (0 = L, 1 = C, 2 = R)
        for (int i = 0; i < framesPerBuffer; i++) {
            for (int j = 0; j < p->channels; j++) {

                // Two-channel playback
                output[k] = p->x[selection][next_sample + k];

                // Increment k
                k++;
            }
        }
        
        // Increment next frame by number of frames written
        p->next_frame[selection] += framesPerBuffer;
    }

    return 0;
}


// AUDIO CALLBACK: END
///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////
