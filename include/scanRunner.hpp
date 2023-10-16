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

#define PSG_DIFF  (0)
#define PSG_JPA   (1)
#define PSG_PROBE (2)
#define NUM_PSGS  (3)

#define NO_FAXION     (0)
#define SHARP_FAXION  (1)
#define BROAD_FAXION  (2)

class ScanRunner {
public:
    ScanRunner(double maxIntegrationTime, int scanType = NO_FAXION, int decisionMaking = 0);
    ~ScanRunner();

    void setTarget(double targetCoupling);

    void acquireData();
    void step(double stepSize);
    void saveData();

    void refreshBaselineAndBadBins(int repeats = 3, int subSpectra = 32, int savePlots = 0);


    // Public parameters
    int subSpectraAveragingNumber;

private:
    // Pumping parameters
    double faxionFreq, faxionPower;
    double xModeFreq, yModeFreq;
    double diffPower, jpaPower;

    // Acquisition Parameters
    double sampleRate, RBW;
    double trueCenterFreq;
    int maxSpectraPerAcquisition;

    // Filter Parameters
    double cutoffFrequency, stopbandAttenuation;
    int poleNumber;

    // Misc variables
    int scanType;
    const char* wisdomFilePath;

    // Member classes
    PSG psgList[NUM_PSGS];
    ATS alazarCard;
    fftw_plan fftwPlan;
    DataProcessor dataProcessor;

    // Threaded structs
    SavedData savedData;
    BayesFactors bayesFactors;
    DecisionAgent decisionAgent;


    // Private methods
    void initPSGs();
    void initAlazarCard();
    void initFFTW();
    void initProcessor();
    void initDecisionAgent(int decisionMaking);

    void acquireProcCalibration(int repeats = 3, int subSpectra = 32, int savePlots = 0);
    void acquireProcCalibrationMulti(int repeats = 3, int subSpectra = 32, int savePlots = 0);
};

#endif // SCANRUNNER_H