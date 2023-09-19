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

#define REFRESH_PROCESSOR (0)

int main() {
    double maxIntegrationTime = 6; // seconds
    double stepSize = 0.1; // MHz
    int numSteps = 50;


    ScanRunner scanRunner(maxIntegrationTime, 0);

    scanRunner.setTarget(8e-5);

    #if REFRESH_PROCESSOR
    scanRunner.refreshBaselineAndBadBins(1, 32, 1);
    #endif

    scanRunner.acquireData();
    for (int i = 0; i < numSteps; i++) {
        scanRunner.step(stepSize);
        scanRunner.acquireData();
    }

    scanRunner.saveData();

    return 0;
}
