/******************************************************************************
 *                                                                            *
 * DECS.H                                                                     *
 *                                                                            *
 * GLOBAL MACROS, FUNCTION DEFINITIONS, INCLUDES, AND DECLARATIONS            *
 *                                                                            *
 ******************************************************************************/

#ifndef DECS_H
#define DECS_H

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
#include "PSG.hpp"
#include "AWG.hpp"
#include "ATS.hpp"
#include "instrument.hpp"
#include "tests.hpp"
#include "multiThreading.hpp"




/*******************************************************************************
 *                                                                            *
 * GLOBAL MACROS AND DEFINITIONS                                               *
 *                                                                            *
 ******************************************************************************/
#define BUFFER_COUNT (8)

#define VERBOSE_OUTPUT (1)