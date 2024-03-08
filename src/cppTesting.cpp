#include "decs.hpp"

int main() {
    // Step information
    int steps(501);
    double integrationTimes[] = {19,19.5,20,20.5,21};

    // Scan parameters
    std::string scanDir = "C:/Users/Lehnert Lab/Desktop/mexDynamicScanning/scanDir/";
    ScanParameters scanParameters;

    scanParameters.topLevelParameters.decisionMaking = false;
    scanParameters.topLevelParameters.baselinePath = scanDir + "baseline/";
    scanParameters.topLevelParameters.statePath = scanDir + "state/";
    scanParameters.topLevelParameters.savePath = scanDir + "save/";
    scanParameters.topLevelParameters.visPath = scanDir + "vis/";
    scanParameters.topLevelParameters.wisdomPath = scanDir + "wisdom/";

    scanParameters.dataParameters.maxIntegrationTime = 7.5;
    scanParameters.dataParameters.sampleRate = 32e6;
    scanParameters.dataParameters.RBW = 100;
    scanParameters.dataParameters.trueCenterFreq = 5e3; // 5 GHz
    scanParameters.dataParameters.stepSize = 0.05; // 50 kHz
    scanParameters.dataParameters.subSpectraAveragingNumber = 20;

    scanParameters.filterParameters.cutoffFrequency = 10e3;
    scanParameters.filterParameters.poleNumber = 3;
    scanParameters.filterParameters.stopbandAttenuation = 15;


    // Actual scan
    for (double integrationTime : integrationTimes) {
        scanParameters.dataParameters.trueCenterFreq = 5e3;
        scanParameters.dataParameters.maxIntegrationTime = integrationTime;
        
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
        }

        std::string jsonString = performanceToJson().dump();
        std::ofstream fileStream(scanParameters.topLevelParameters.savePath + "performances/perf_" + std::to_string(integrationTime) + ".json");
        fileStream << jsonString;
        fileStream.close();

        deleteAllFilesInFolder(scanParameters.topLevelParameters.statePath);

        resetMetrics();
        resetTimers();
    }
}