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
#define BUFFER_COUNT (8)

#define VERBOSE_OUTPUT (1)


/*******************************************************************************
 *                                                                            *
 * LIBRARY INCLUDES                                                           *
 *                                                                            *
 ******************************************************************************/

// Standard library includes
#include <stdio.h>
#include <conio.h>
#include <cmath>
#include <algorithm>

#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>

#include <string>
#include <vector>
#include <queue>

#include <chrono>
#include <stdexcept>
#include <future>

#include <thread>
#include <condition_variable>
#include <mutex>


// Custom library includes
#include <fftw3.h>

#include <visa.h>

#include "AlazarError.h"
#include "AlazarApi.h"
#include "AlazarCmd.h"
#include "IoBuffer.h"

#include "H5Cpp.h"

#include "utils/matplotlibcpp.h"
namespace plt = matplotlibcpp;


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
    std::queue<fftw_complex*> processedDataQueue;

    std::condition_variable dataReadyCondition;
    std::condition_variable saveReadyCondition;
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
 * FUNCTION DECLARATIONS                                                      *
 *                                                                            *
 ******************************************************************************/

// tests.cpp
void printAvailableResources();
void psgTesting(int gpibAdress);
void awgTesting(int gpibAdress);

// multiThreading.cpp
void processingThread(fftw_plan plan, int N, SharedData& sharedData, SynchronizationFlags& syncFlags);
void decisionMakingThread(SharedData& sharedData, SynchronizationFlags& syncFlags);
void saveDataToBin(SharedData& sharedData, SynchronizationFlags& syncFlags);
void saveDataToHDF5(SharedData& sharedData, SynchronizationFlags& syncFlags);


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

#endif // DECS_H