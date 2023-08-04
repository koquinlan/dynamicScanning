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

#define VERBOSE_OUTPUT (0)

#define _USE_MATH_DEFINES

// Timers
#define TIMER_ACQUISITION   (0)
#define TIMER_FFT           (1)
#define TIMER_MAG           (2)
#define TIMER_DECISION      (3)
#define TIMER_PROCESS       (4)
#define TIMER_SAVE          (5)
#define NUM_TIMERS          (6)


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

#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
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


// Custom library includes
#include <fftw3.h>

#include <Eigen/Dense>

#include <visa.h>

#include "AlazarError.h"
#include "AlazarApi.h"
#include "AlazarCmd.h"
#include "IoBuffer.h"

// #include "H5Cpp.h"

// #include "matplotlibcpp.h"
// namespace plt = matplotlibcpp;

#include "DspFilters/Dsp.h"


/*******************************************************************************
 *                                                                            *
 * STRUCTS AND GLOBALS                                                        *
 *                                                                            *
 ******************************************************************************/

// Struct for storing data shared between threads. Used for multithreaded data acquisition.
struct SharedData {
    std::mutex mutex;

    int samplesPerBuffer;

    std::queue<fftw_complex*> dataQueue;
    std::queue<fftw_complex*> dataSavingQueue;
    std::queue<fftw_complex*> FFTDataQueue;
    std::queue<std::vector<double>> processedDataQueue;

    std::condition_variable dataReadyCondition;
    std::condition_variable saveReadyCondition;
    std::condition_variable FFTDataReadyCondition;
    std::condition_variable processedDataReadyCondition;
};

// Struct for storing synchronization flags. Used for multithreaded data acquisition.
struct SynchronizationFlags {
    std::mutex mutex;
    bool pauseDataCollection;
    bool acquisitionComplete;
    bool dataReady;
    bool dataProcessingComplete;

    SynchronizationFlags() : pauseDataCollection(false), acquisitionComplete(false),
                             dataReady(false), dataProcessingComplete(false) {}
};


/*******************************************************************************
 *                                                                            *
 * CLASS DEFINITIONS                                                          *
 *                                                                            *
 ******************************************************************************/

// Class includes
#include "instruments/instrument.hpp"

#include "instruments/PSG.hpp"
#include "instruments/AWG.hpp"
#include "instruments/ATS.hpp"

#include "dataProcessing/dataProcessor.hpp"


/*******************************************************************************
 *                                                                            *
 * FUNCTION DECLARATIONS                                                      *
 *                                                                            *
 ******************************************************************************/

// dataProcessingUtils.cpp
std::vector<double> averageVectors(std::vector<std::vector<double>> vecs);
int findClosestIndex(std::vector<double> vec, double target);
std::vector<int> findOutliers(const std::vector<double>& data, int windowSize = 10, double multiplier = 3);
int findMaxIndex(std::vector<double> vec, int startIndex, int endIndex);
void unwrapPhase(std::vector<double>& phase);
std::tuple<double, double> vectorStats(const std::vector<double>& vec);
void trimVector(std::vector<double>& vec, double cutPercentage);

// fileIO.cpp
std::vector<std::vector<double>> readCSV(std::string filename, int maxLines);
std::vector<double> readVector(const std::string& filename);
void saveVector(std::vector<int> data, std::string filename);
void saveVector(std::vector<double> data, std::string filename);

// multiThreading.cpp
void decisionMakingThread(SharedData& sharedData, SynchronizationFlags& syncFlags);
void FFTThread(fftw_plan plan, int N, SharedData& sharedData, SynchronizationFlags& syncFlags);
void magnitudeThread(int N, SharedData& sharedData, SynchronizationFlags& syncFlags, DataProcessor& dataProcessor);
void saveDataToBin(SharedData& sharedData, SynchronizationFlags& syncFlags);
void saveDataToHDF5(SharedData& sharedData, SynchronizationFlags& syncFlags);

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
void reportPerformance();

#endif // DECS_H