#include "decs.hpp"

void staticRun(ScanParameters scanParameters);
void dynamicRun(ScanParameters scanParameters);

int main() {
    // Scan parameters
    std::string scanDir = "C:/Users/Lehnert Lab/Desktop/mexDynamicScanning/scanDir/";
    ScanParameters scanParameters;

    scanParameters.topLevelParameters.baselinePath = scanDir + "baseline/";
    scanParameters.topLevelParameters.statePath = scanDir + "state/";
    scanParameters.topLevelParameters.savePath = scanDir + "save/";
    scanParameters.topLevelParameters.visPath = scanDir + "vis/";
    scanParameters.topLevelParameters.wisdomPath = scanDir + "wisdom/";

    scanParameters.dataParameters.sampleRate = 32e6;
    scanParameters.dataParameters.RBW = 100;
    scanParameters.dataParameters.trueCenterFreq = 5e3; // 5 GHz
    scanParameters.dataParameters.stepSize = 0.1; // 100 kHz
    scanParameters.dataParameters.subSpectraAveragingNumber = 20;

    scanParameters.filterParameters.cutoffFrequency = 10e3;
    scanParameters.filterParameters.poleNumber = 3;
    scanParameters.filterParameters.stopbandAttenuation = 15;



    scanParameters.dataParameters.maxIntegrationTime = 10;
    scanParameters.dataParameters.minIntegrationTime = 10;
    scanParameters.topLevelParameters.decisionMaking = false;

    ScanRunner scanRunner(scanParameters);
    scanRunner.refreshBaselineAndBadBins();

    // staticRun(scanParameters);    
    dynamicRun(scanParameters);
}

void staticRun(ScanParameters scanParameters) {
    int steps(200);
    double integrationTimes[] = {10.5};

    scanParameters.topLevelParameters.decisionMaking = false;


    // Actual scan
    for (double integrationTime : integrationTimes) {
        scanParameters.dataParameters.trueCenterFreq = 5.0101e3 + scanParameters.dataParameters.stepSize;
        scanParameters.dataParameters.maxIntegrationTime = integrationTime;
        scanParameters.dataParameters.minIntegrationTime = integrationTime;
        
        for (int i = 0; i < steps; i++) {
            ScanRunner scanRunner(scanParameters);

            std::ifstream file(scanParameters.topLevelParameters.statePath + "scanInfo.json");
            if (file.is_open()) {
                file.close();
                scanRunner.loadStateAndStep();
            }
            else { file.close(); }
        
            scanRunner.acquireData();
            scanRunner.saveState();

            if (i % 10 == 0){
                scanRunner.saveData();
            }

            scanParameters.dataParameters.trueCenterFreq += scanParameters.dataParameters.stepSize;

            std::cout << "Step " << i << " of " << steps << " with integration time " << integrationTime << " complete.\n";

            // if (i == steps - 1) {
            //     scanRunner.saveState(std::to_string(integrationTime) + "_");
            // }
        }

        std::string jsonString = performanceToJson().dump();
        std::ofstream fileStream(scanParameters.topLevelParameters.savePath + "performances/perf_" + std::to_string(integrationTime) + ".json");
        fileStream << jsonString;
        fileStream.close();

        // deleteAllFilesInFolder(scanParameters.topLevelParameters.statePath);

        resetMetrics();
        resetTimers();
    }
}


void dynamicRun(ScanParameters scanParameters) {
    // Load the seed run
    scanParameters.dataParameters.trueCenterFreq = 5.0301e3 + scanParameters.dataParameters.stepSize;

    int steps(1001);
    double thresholds[] = {1.3};

    scanParameters.topLevelParameters.decisionMaking = true;
    scanParameters.topLevelParameters.targetCoupling = 1.45e-3;

    scanParameters.dataParameters.maxIntegrationTime = 11;
    scanParameters.dataParameters.minIntegrationTime = 8.5;


    // Actual scan
    for (double threshold : thresholds) {
        for (int i = 0; i < steps; i++) {
            ScanRunner scanRunner(scanParameters);
            scanRunner.decisionAgent.threshold = threshold;

            std::ifstream file(scanParameters.topLevelParameters.statePath + "scanInfo.json");
            if (file.is_open()) {
                file.close();
                scanRunner.loadStateAndStep();
            }
            else { file.close(); }

            scanRunner.acquireData();
            // scanRunner.decisionAgent.saveState(scanParameters.topLevelParameters.statePath);
            scanRunner.saveState();

            if (i % 10 == 0){
                scanRunner.saveData();
            }

            scanParameters.dataParameters.trueCenterFreq += scanParameters.dataParameters.stepSize;

            std::cout << "Step " << i << " of " << steps << 
                      " with max integration time " << scanParameters.dataParameters.maxIntegrationTime << 
                      " and threshold " << threshold <<
                      " complete.\n";
        }

        std::string jsonString = performanceToJson().dump();
        std::ofstream fileStream(scanParameters.topLevelParameters.savePath + "performances/perf_" + std::to_string(threshold) + "_dynamic.json");
        fileStream << jsonString;
        fileStream.close();

        resetMetrics();
        resetTimers();
    }
}