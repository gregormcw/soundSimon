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

#define MAX_ROUNDS          100
#define MAX_PATH_LEN        256
#define MAX_FILES           8
#define MAX_CHAN            2
#define FRAMES_PER_BUFFER   1024 
#define DEBUG               0


// Callback function prototype
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

    // OPEN LIST OF FILES

    // Direct file pointer to command line argument
    fp = fopen(argv[1], "r");

    // If fp points to null value, print error statement and return -1
    if (!fp) {
        fprintf(stderr, "Cannot open %s\n", argv[1]);
        return -1;
    }

    // read WAV filenames from the list in ifile_list.txt
    // print information about each WAV file


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

        // Set number of input files to 0
        num_input_files = 0;
        
        // Retrieve frame information and assign to Buf structure
        p->frames[i] = sfinfo.frames;

        // Set next frame to 0
        p->next_frame[i] = 0;

        // Set playback flag to false
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

    // Close list of input files
    fclose(fp);


// AUDIO FILE READ: END
///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////


// GAME ENGINE: START
///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////


    // Use time as seed for generation of random values
    srand(time(0));

    // Set initial struct member variable values
    gp->round = 0;
    p->round = 0;
    gp->isPlaying = 1;
    gp->lastInputValid = 1;
    p->selection = -1;
    p->readyForUserInput = 1;

    // Start PortAudio
    stream = startupPa(1, p->channels, p->samplerate, FRAMES_PER_BUFFER, 
        paCallback, &buf);

    // Initialize ncurses screen
    initscr();
    cbreak();

    // Do not display user input
    noecho();

    // Display start screen
    printStartScreen();

    // Refreshes screen to match what is in memory
    refresh();

    // Wait for user input, and assign to type int
    int c = getch();

    // If user does not begin or quit game, display that input is invalid
    while (c != ' ' && c != 'p') {
        clear();
        printStartScreen();
        printw("%c is not a valid key.\n", c);
        c = getch();
    }

    // When input is valid, refresh start screen
    clear();
    printStartScreen();

    // If user presses the space bar, begin game and provide notification
    if (c == ' ') {

        printw("         Starting now. Get ready!\n");
        // p->selection = 3;

        // Loop while isPlaying evaluates to true
        while (gp->isPlaying) {

            // If user has completed round, display congratulatory notification
            if (gp->round > 0) {
                clear();
                printStartScreen();
                printw("Congratulations! You made it through round %d\n", gp->round);
                // p->selection = 4;
            }

            // If, last user input was valid, generate new game sequence value
            if (gp->lastInputValid)
                newGameArrayVal(gp, p, stream);

            // Play all array values to user
            printGameState(gp, p, stream);

            // Protect against early user input during playback
            while (!p->readyForUserInput) {
                // Do nothing
            }

            // For each round, wait for user input and compare against
            // correct sequence value at that index
            for (int i = 0; i < gp->round+1; i++) {

                c = getch();
                // p->selection = intToIdx(c);

                // If user presses 'p', quit game
                if (c == 'p') {
                    clear();
                    printStartScreen();
                    printw("   You scored %d! Thank you for playing!\n", gp->round);
                    gp->isPlaying = 0;
                    break;
                }

                // If user presses invalid key, display notification
                while (!isValid(c)) {
                    gp->lastInputValid = 0;
                    clear();
                    printStartScreen();
                    printw("\t  %c is not a valid key\n", c);
                    c = getch();
                }

                // Otherwise, last user input was valid, so set flag to true
                gp->lastInputValid = 1;

                // If user input does not match the correct game sequence value, 
                // notify that game is over, set isPlayinf flag to false, and 
                // break out of while loop
                if (c != gp->gameArray[i]) {
                    gp->isPlaying = 0;
                    printw("\n  Wrong input. GAME OVER. You scored %d!\n", gp->round);
                    // p->selection = 5;
                    break;

                }

                // Otherwise, user input was correct for that index, so
                // display notification
                else if (c == gp->gameArray[i]) {
                    printw("          Correct! You entered: %c\n", c);

                }             
            }

            // Increment round by 1 in both structs
            gp->round += 1;
            p->round = gp->round;
        }

        // If game is over, user may hit any key to exit
        printw("           Hit any key to exit\n");
        c = getch();
    }

    // Deallocate memory and close ncurses
    endwin();

    // Shut down PortAudio
    shutdownPa(stream);

    return 0;
}


// MAIN(), GAME ENGINE: END
///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////


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

    // If playback flag is false, output only zeros
    if (!p->playback) {
        for (int i = 0; i < framesPerBuffer * p->channels; i++) {
            output[i] = 0.0;
        }
    }

    else {

        // Otherwise, selected file is determined by game sequence value at
        // current round, which will have range [0, round-1]
        selection = intToIdx(p->gameArray[p->curRound]);

        // Declare and initialize variables
        int next_sample, k = 0;
        int endOfBuff = p->next_frame[selection] + framesPerBuffer;

        // Determine end of sequence and current sequence index
        // relative to curRound
        unsigned long endOfSeq = (p->round+1) * p->frames[0];

        unsigned long seqIndex = p->curRound * p->frames[0] 
            + p->next_frame[selection] + framesPerBuffer;

        // If sequence index is beyond the end of the entire game sequence,
        // all audio has been played and playback is set to false
        if (seqIndex >= endOfSeq) {

            // Reset file pointer
            p->next_frame[selection] = 0;

            // Turn off audible playback
            p->playback = 0;

            // Communicate to main() that game is ready for user input
            p->readyForUserInput = 1;
        }

        // If sequence index is >= current audio file, but NOT >= the entire
        // game sequence, continue to next game sequence value
        if (endOfBuff >= p->frames[selection] && endOfBuff < endOfSeq) {

            // Reset file pointer
            p->next_frame[selection] = 0;

            // Determine next file
            p->selection = intToIdx(p->gameArray[p->curRound + 1]);

            // Increment curRound
            p->curRound++;
        }

        // Set next sample as next frame * number of channels
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


// AUDIO CALLBACK: END ====================== THANKS FOR READING!!!
///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////
