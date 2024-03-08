/**
 * @file decs.hpp
 * @author Kyle Quinlan (kyle.quinlan@colorado.edu)
 * @brief Defines global macros, includes and function prototypes for the project.
 * @version 0.1
 * @date 2023-06-30
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#ifndef DECS_H
#define DECS_H

/*******************************************************************************
 *                                                                            *
 * GLOBAL MACROS AND DEFINITIONS                                               *
 *                                                                            *
 ******************************************************************************/
#define _CRTDBG_MAP_ALLOC // leak detection

#define BUFFER_COUNT (8)

#define _USE_MATH_DEFINES

// Timers
#define TIMER_ACQUISITION   (0)
#define TIMER_FFT           (1)
#define TIMER_MAG           (2)
#define TIMER_AVERAGE       (3)
#define TIMER_PROCESS       (4)
#define TIMER_DECISION      (5)
#define TIMER_SAVE          (6)
#define NUM_TIMERS          (7)

// Per spectrum timing
#define ACQUIRED_SPECTRA (0)
#define SPECTRA_AT_DECISION (1)
#define SPECTRUM_AVERAGE_SIZE (2)
#define NUM_METRICS (3)

// Data saving flags
#define SAVE_PROGRESS (0)


/*******************************************************************************
 *                                                                            *
 * LIBRARY INCLUDES                                                           *
 *                                                                            *
 ******************************************************************************/

// Standard library includes
#include <stdio.h>
#include <crtdbg.h> // for leak detection
#include <conio.h>
#include <cmath>
#include <algorithm>
#include <numeric>
#include <windows.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>

#include <string>
#include <vector>
#include <queue>
#include <complex>
#include <iterator>

#include <chrono>
#include <stdexcept>
#include <future>

#include <thread>
#include <condition_variable>
#include <mutex>
#include <memory>
#include <atomic>


// Custom library includes
#include <fftw3.h>
#include "DspFilters/Dsp.h"
#include "utils/json.hpp"
using json = nlohmann::json;

#include "AlazarError.h"
#include "AlazarApi.h"
#include "AlazarCmd.h"
#include "IoBuffer.h"

// #include "H5Cpp.h"

// #include "matplotlibcpp.h"
// namespace plt = matplotlibcpp;


/*******************************************************************************
 *                                                                            *
 * STRUCTS AND GLOBALS                                                        *
 *                                                                            *
 ******************************************************************************/
// Structs for scanning parameters
struct TopLevelParameters {
    bool decisionMaking;
    std::string baselinePath;
    std::string statePath;
    std::string savePath;
    std::string visPath;
    std::string wisdomPath;
};

struct DataParameters {
    double maxIntegrationTime;
    double sampleRate;
    double RBW;
    double trueCenterFreq;
    int subSpectraAveragingNumber;
};

struct FilterParameters {
    double cutoffFrequency;
    int poleNumber;
    double stopbandAttenuation;
};

struct ScanParameters {
    TopLevelParameters topLevelParameters;
    DataParameters dataParameters;
    FilterParameters filterParameters;
};


// Struct for holding spectrum information
struct Spectrum {
    std::vector<double> powers;
    std::vector<double> freqAxis;
    
    double trueCenterFreq;
};

struct CombinedSpectrum : public Spectrum {
    std::vector<double> weightSum;
    std::vector<double> sigmaCombined;
    std::vector<int> numTraces;
};


// Struct for storing data shared between threads. Used for multithreaded data acquisition.
struct SharedDataBasic{
    std::mutex mutex;

    int samplesPerBuffer;

    std::queue<fftw_complex*> dataQueue;
    std::queue<fftw_complex*> backupDataQueue;
    std::queue<fftw_complex*> dataSavingQueue;
    std::queue<fftw_complex*> FFTDataQueue;

    std::condition_variable dataReadyCondition;
    std::condition_variable saveReadyCondition;
    std::condition_variable FFTDataReadyCondition;
};

struct SharedDataProcessing {
    std::mutex mutex;

    std::queue<std::vector<double>> magDataQueue;
    std::queue<Spectrum> rawDataQueue;
    std::queue<CombinedSpectrum> rebinnedDataQueue;

    std::condition_variable magDataReadyCondition;
    std::condition_variable rawDataReadyCondition;
    std::condition_variable rescaledDataReadyCondition;

};

struct SharedDataSaving {
    std::mutex mutex;

    std::queue<Spectrum> averagedSpectrumQueue;
    std::condition_variable averagedSpectrumReadyCondition;
};

struct AveragedData {
    std::vector<double> collatedData;
    size_t numSpectra;
    size_t spectrumLength;
};


// Struct for saving data
struct SavedData{
    std::mutex mutex;

    std::vector<Spectrum> rawSpectra;
    std::vector<Spectrum> processedSpectra;
    std::vector<Spectrum> rescaledSpectra;

    CombinedSpectrum combinedSpectrum;
};

// Struct for storing synchronization flags. Used for multithreaded data acquisition.
struct SynchronizationFlags {
    std::mutex mutex;
    bool pauseDataCollection;

    bool acquisitionComplete;
    bool FFTComplete;
    bool magnitudeComplete;
    bool averagingComplete;
    bool processingComplete;
    bool decisionsComplete;

    bool errorFlag;
    std::string errorMessage;

    SynchronizationFlags() : pauseDataCollection(false), acquisitionComplete(false),
                             FFTComplete(false), magnitudeComplete(false), 
                             averagingComplete(false), processingComplete(false),
                             decisionsComplete(false),
                             errorFlag(false), errorMessage("") {}
};


/*******************************************************************************
 *                                                                            *
 * CLASS DEFINITIONS                                                          *
 *                                                                            *
 ******************************************************************************/

// Class includes
#include "utils/multiThreading.hpp"

#include "instruments/ATS.hpp"

#include "dataProcessing/bayes.hpp"
#include "dataProcessing/dataProcessor.hpp"

#include "decisionAgent.hpp"
#include "scanRunner.hpp"


/*******************************************************************************
 *                                                                            *
 * FUNCTION DECLARATIONS                                                      *
 *                                                                            *
 ******************************************************************************/

// dataProcessingUtils.cpp
std::vector<double> averageVectors(const std::vector<std::vector<double>>& vecs);
int findClosestIndex(std::vector<double> vec, double target);
std::vector<int> findOutliers(const std::vector<double>& data, int windowSize = 50, double multiplier = 5);
int findMaxIndex(std::vector<double> vec, int startIndex, int endIndex);
void unwrapPhase(std::vector<double>& phase);
std::tuple<double, double> vectorStats(std::vector<double> vec);
void trimVector(std::vector<double>& vec, double cutPercentage);
void trimSpectrum(Spectrum& spec, double cutPercentage);

// fileIO.cpp
std::vector<std::vector<double>> readCSV(std::string filename, int maxLines);
Spectrum readSpectrum(std::string filename);
CombinedSpectrum readCombinedSpectrum(std::string filename);
std::vector<double> readVector(const std::string& filename);
void saveCombinedSpectrum(CombinedSpectrum data, std::string filename);
void saveSpectrum(Spectrum data, std::string filename);
void saveVector(std::vector<int> data, std::string filename);
void saveVector(std::vector<double> data, std::string filename);
std::string getDateTimeString();
void saveSpectraFromQueue(std::queue<Spectrum>& spectraQueue, std::string filename);

// mexUtils.cpp
ScanParameters unpackScanParameters(json const& inputParams);

// multiThreading.cpp
void fftThread(fftw_plan plan, int samplesPerSpectrum, ThreadSafeQueue<fftw_complex*>& inputQueue, ThreadSafeQueue<fftw_complex*>& outputQueue);
void magnitudeThread(int samplesPerSpectrum, DataProcessor& dataProcessor, ThreadSafeQueue<fftw_complex*>& inputQueue, ThreadSafeQueue<std::vector<double>>& outputQueue);
void averagingThread(DataProcessor& dataProcessor, double trueCenterFreq, ThreadSafeQueue<std::vector<double>>& inputQueue, ThreadSafeQueue<Spectrum>& outputQueue, int subSpectraAveragingNumber);
void processingThread(DataProcessor& dataProcessor, ThreadSafeQueue<Spectrum>& inputQueue, ThreadSafeQueue<CombinedSpectrum>& outputQueue);
void decisionMakingThread(BayesFactors& bayesFactors, DecisionAgent& decisionAgent, ThreadSafeQueue<CombinedSpectrum>& inputQueue, std::atomic<bool>& triggerEnd);

// tests.cpp
void printAvailableResources();
void psgTesting(int gpibAdress);
void awgTesting(int gpibAdress);

//timing.cpp
void setTime(int timerCode, double val);
double getTime(int timerCode);
void startTimer(int timerCode);
void stopTimer(int timerCode);
void resetTimers();
void setMetric(int metricCode, int val);
void updateMetric(int metricCode, int val);
std::vector<int> getMetric(int metricCode);
void reportPerformance();
json performanceToJson();

#endif // DECS_H