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

static std::vector<int> metrics[NUM_METRICS];

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


void setMetric(int metricCode, int val) {
    metrics[metricCode].push_back(val);
}

void updateMetric(int metricCode, int val) {
    metrics[metricCode].back() = val;
}

std::vector<int> getMetric(int metricCode) {
    return metrics[metricCode];
}

// Report a running average of timing data
void reportPerformance()
{
    fprintf(stdout, "\n********** ABSOLUTE PERFORMANCE **********\n");

    fprintf(stdout, "   DATA ACQUISITION:    %8.4g s\n", times[TIMER_ACQUISITION]);
    fprintf(stdout, "   FOURIER TRANSFORM:   %8.4g s\n", times[TIMER_FFT]);
    fprintf(stdout, "   FFT MAGNITUDE:       %8.4g s\n", times[TIMER_MAG]);
    fprintf(stdout, "   AVERAGING:           %8.4g s\n", times[TIMER_AVERAGE]);
    fprintf(stdout, "   PROCESSING:          %8.4g s\n", times[TIMER_PROCESS]);
    fprintf(stdout, "   DECISION MAKING:     %8.4g s\n", times[TIMER_DECISION]);
    fprintf(stdout, "   DATA SAVING:         %8.4g s\n", times[TIMER_SAVE]);

    fprintf(stdout, "*********************************\n\n");


    // Calculate some per spectrum statistics
    int totalAcquiredSpectra = 0;
    double averageDecisionEnforcementDelay = 0;
    int numDecisions = 0;

    for(std::size_t i=0; i<metrics[SPECTRA_AT_DECISION].size(); i++){
        totalAcquiredSpectra += metrics[ACQUIRED_SPECTRA][i];

        if (metrics[SPECTRA_AT_DECISION][i] > 0) {
            numDecisions++;

            averageDecisionEnforcementDelay += metrics[ACQUIRED_SPECTRA][i] - metrics[SPECTRUM_AVERAGE_SIZE][i]*metrics[SPECTRA_AT_DECISION][i];
        }
    }

    averageDecisionEnforcementDelay /= (double)numDecisions;


    fprintf(stdout, "\n********** PER SPECTRUM PERFORMANCE **********\n");

    fprintf(stdout, "   ACQUIRED SPECTRA:                     %d \n", totalAcquiredSpectra);
    fprintf(stdout, "   AVERAGE ACQUISITION TIME:             %8.4g \n", times[TIMER_ACQUISITION]/(double)totalAcquiredSpectra);
    fprintf(stdout, "   AVERAGE DECISION ENFORCEMENT DELAY:   %8.4g \n", averageDecisionEnforcementDelay);

    fprintf(stdout, "*********************************\n\n");
}



// Serialize the timing and metric data into a JSON string
json performanceToJson() {
    json jsonPerf;

    // Serialize timing data
    json timingData;
    
    timingData["acquisition"] = times[TIMER_ACQUISITION];
    timingData["fft"] = times[TIMER_FFT];
    timingData["magnitude"] = times[TIMER_MAG];
    timingData["averaging"] = times[TIMER_AVERAGE];
    timingData["processing"] = times[TIMER_PROCESS];
    timingData["decision"] = times[TIMER_DECISION];
    timingData["save"] = times[TIMER_SAVE];

    jsonPerf["timers"] = timingData;


    // Serialize metric data
    json metricData;

    metricData["acquiredSpectra"] = metrics[ACQUIRED_SPECTRA];
    metricData["spectraAtDecision"] = metrics[SPECTRA_AT_DECISION];
    metricData["spectrumAverageSize"] = metrics[SPECTRUM_AVERAGE_SIZE];

    jsonPerf["metrics"] = metricData;


    return jsonPerf;
}
