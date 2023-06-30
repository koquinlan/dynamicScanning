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
 * INCLUDES                                                                   *
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

#include "matplotlibcpp.h"
namespace plt = matplotlibcpp;


// Class includes
#include "instrument.hpp"

#include "PSG.hpp"
#include "AWG.hpp"
#include "ATS.hpp"


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


#endif // DECS_H