/**
 * @file scanRunner.hpp
 * @author Kyle Quinlan (kyle.quinlan@colorado.edu)
 * @brief Class definition for ScanRunner class. This class is responsible for running a single scan, including data acquisition, 
 *        processing, stepping, and saving.
 * @version 0.1
 * @date 2023-08-10
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#ifndef SCANRUNNER_H
#define SCANRUNNER_H

#include "decs.hpp"

#define PSG_DIFF (0)
#define PSG_JPA (1)
#define PSG_PROBE (2)
#define NUM_PSGS (3)

class ScanRunner {
public:
    ScanRunner();
    ~ScanRunner();

    void acquireData();
    void saveData();

private:
    // Pumping parameters
    double xModeFreq, yModeFreq;
    double diffPower, jpaPower;

    // Acquisition Parameters
    double sampleRate, RBW;
    int maxSpectraPerAcquisition;

    // Filter Parameters
    double cutoffFrequency, stopbandAttenuation;
    int poleNumber;

    // Misc variables
    const char* wisdomFilePath;

    // Member classes
    PSG psgList[NUM_PSGS];
    ATS alazarCard;
    fftw_plan fftwPlan;
    DataProcessor dataProcessor;

    // Threaded structs
    SavedData savedData;
    BayesFactors bayesFactors;


    // Private methods
    void initPSGs();
    void initAlazarCard();
    void initFFTW();
    void initProcessor();
};

#endif // SCANRUNNER_H