/**
 * @file timing.cpp
 * @author Kyle Quinlan (kyle.quinlan@colorado.edu)
 * @brief 
 * @version 0.1
 * @date 2023-07-28
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include "decs.hpp"

static double timers[NUM_TIMERS];
static double times[NUM_TIMERS] = {0};

void setTime(int timerCode, double val) {
    times[timerCode] = val;
}

double getTime(int timerCode) { 
    return times[timerCode]; 
}

void startTimer(int timerCode) {
    timers[timerCode] = GetTickCount();
}

void stopTimer(int timerCode) {
    times[timerCode] += (GetTickCount() - timers[timerCode])/1000;
}

void resetTimers()
{
    for (int n = 0; n < NUM_TIMERS; n++) {
        times[n] = 0.;
    }
}

// Report a running average of timing data
void reportPerformance()
{
    fprintf(stdout, "\n********** PERFORMANCE **********\n");

    fprintf(stdout, "   DATA ACQUISITION:    %8.4g s\n", times[TIMER_ACQUISITION]);
    fprintf(stdout, "   FOURIER TRANSFORM:   %8.4g s\n", times[TIMER_FFT]);
    fprintf(stdout, "   FFT MAGNITUDE:       %8.4g s\n", times[TIMER_MAG]);
    fprintf(stdout, "   DECISION MAKING:     %8.4g s\n", times[TIMER_DECISION]);
    #if SAVE_DATA
    fprintf(stdout, "   DATA SAVING:         %8.4g s\n", times[TIMER_SAVE]);
    #endif

    fprintf(stdout, "*********************************\n\n");
}
