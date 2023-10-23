/**
 * @file baselineTests.cpp
 * @author Kyle Quinlan (kyle.quinlan@colorado.edu)
 * @brief For rapid prototyping and testing of baselining procedure
 * @version 0.1
 * @date 2023-10-17
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#include "decs.hpp"

int main() {
    int maxSpectraPerStep = 50;
    int minSpectraPerStep = 20;
    int subSpectraAveragingNumber = 15;
    double maxIntegrationTime = maxSpectraPerStep*subSpectraAveragingNumber*0.01; // seconds

    double stepSize = 0.1; // MHz
    int numSteps = 50;


    ScanRunner scanRunner(maxIntegrationTime, 0, 0);
    scanRunner.subSpectraAveragingNumber = subSpectraAveragingNumber;

    scanRunner.refreshBaselineAndBadBins(1, 32, 1);

    return 0;
}
