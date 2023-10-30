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


void dynamicRun(int maxSpectraPerStep, int minSpectraPerStep, int subSpectraAveragingNumber, double stepSize, int numSteps, double targetCoupling);
void staticRun(int maxSpectraPerStep, int subSpectraAveragingNumber, double stepSize, int numSteps, double targetCoupling);

int main() {
    int steps(500), subSpectraAveragingNumber(15);
    double couplingTarget(3.4e-5), stepSize(0.1);

    // int maxSpectraPerStep, int minSpectraPerStep, int subSpectraAveragingNumber, double stepSize, int numSteps, double targetCoupling
    dynamicRun(75, 45, subSpectraAveragingNumber, stepSize, steps, couplingTarget);


    int staticMaxSpectra[] = {46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57};
    // int maxSpectraPerStep, int subSpectraAveragingNumber, double stepSize, int numSteps, double targetCoupling
    for (int maxSpectra : staticMaxSpectra) {
        try
        {
            staticRun(maxSpectra, subSpectraAveragingNumber, stepSize, steps, couplingTarget);
        }
        catch(const std::exception& e)
        {
            std::cerr << e.what() << '\n';
        }
    }

    return 0;
}


void dynamicRun(int maxSpectraPerStep, int minSpectraPerStep, int subSpectraAveragingNumber, double stepSize, int numSteps, double targetCoupling){
    double maxIntegrationTime = maxSpectraPerStep*subSpectraAveragingNumber*0.01; // seconds

    ScanRunner scanRunner(maxIntegrationTime, 0, 1);
    scanRunner.subSpectraAveragingNumber = subSpectraAveragingNumber;
    scanRunner.setTarget(targetCoupling);
    scanRunner.decisionAgent.minShots = minSpectraPerStep;


    #if REFRESH_PROCESSOR
    scanRunner.refreshBaselineAndBadBins(1, 32, 1);
    #endif

    scanRunner.acquireData();
    for (int i = 0; i < numSteps; i++) {
        scanRunner.step(stepSize);
        scanRunner.acquireData();
    }

    scanRunner.saveData(1);
}


void staticRun(int maxSpectraPerStep, int subSpectraAveragingNumber, double stepSize, int numSteps, double targetCoupling){
    double maxIntegrationTime = maxSpectraPerStep*subSpectraAveragingNumber*0.01; // seconds

    ScanRunner scanRunner(maxIntegrationTime, 0, 0);
    scanRunner.subSpectraAveragingNumber = subSpectraAveragingNumber;
    scanRunner.setTarget(targetCoupling);

    scanRunner.acquireData();
    for (int i = 0; i < numSteps; i++) {
        scanRunner.step(stepSize);
        scanRunner.acquireData();
    }

    scanRunner.saveData();
}

