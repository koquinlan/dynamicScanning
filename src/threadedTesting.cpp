/**
 * @file threadedTesting.cpp
 * @author Kyle Quinlan (kyle.quinlan@colorado.edu)
 * @brief For rapid prototyping and testing of multithreaded control and acquisition code. See src/controlTests.cpp for single threaded development.
 * @version 0.1
 * @date 2023-06-30
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#include "decs.hpp"

#define REFRESH_PROCESSOR (1)

int main() {
    ScanRunner scanRunner;

    #if REFRESH_PROCESSOR
    scanRunner.refreshBaselineAndBadBins(1, 32, 1);
    #endif

    scanRunner.acquireData();
    scanRunner.saveData();

    return 0;
}
