/**
 * @file controlTests.cpp
 * @author Kyle Quinlan (kyle.quinlan@colorado.edu)
 * @brief For rapid prototyping and testing of single threaded control and acquisition code. See src/threadedTesting.cpp for multithreaded development.
 * @version 0.1
 * @date 2023-06-30
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include "decs.hpp"

std::tuple<double, double> getVisibility(std::vector<double> fftPowerProbeOn, std::vector<double> fftPowerBackground, std::vector<double> probeFreqs, std::vector<double> freqAxis, double probeFreq, double yModeFreq);

int main() {
    // Initialize scan runners
    int maxSpectra(32), subSpectraAveragingNumber(15); 
    double maxIntegrationTime = maxSpectra*subSpectraAveragingNumber*0.01;

    ScanRunner scanRunner(maxIntegrationTime, 0, 0);
    scanRunner.subSpectraAveragingNumber = subSpectraAveragingNumber;


    // Probe parameters
    PSG psgProbe(27);
    psgProbe.setPow(-95);

    double probeSpan = 30; // MHz
    int numProbes = 200;

    double xModeFreq = 5.208; // GHz

    std::vector<double> probeFreqs(numProbes);
    for (int i = 0; i < numProbes/2; ++i) {
        probeFreqs[2*i] = xModeFreq  - (probeSpan/2*1e-3 - i*probeSpan/(numProbes-1)*1e-3);
        probeFreqs[2*i + 1] = xModeFreq  + (probeSpan/2*1e-3 - i*probeSpan/(numProbes-1)*1e-3);
        std::cout << (probeFreqs[2*i] - xModeFreq )*1e3 << " " << (probeFreqs[2*i+1] - xModeFreq )*1e3 << " ";
    }
    std::cout << std::endl;


    // Acquire data
    std::vector<double> visibility, trueProbeFreqs;
    DataProcessor proc;
    for (double probe : probeFreqs) {
        std::cout << std::endl
          << "Taking data at " << std::fixed << std::setprecision(6) << probe
          << " GHz, " << std::fixed << std::setprecision(2)
          << (trueProbeFreqs.size() / static_cast<double>(numProbes)) * 100
          << " % complete\r" << std::endl << std::endl;


        psgProbe.setFreq(probe);
        psgProbe.onOff(true);
        scanRunner.acquireData();
        std::vector<double> fftPowerProbeOn = averageVectors(scanRunner.retrieveRawData());
        scanRunner.flushData();


        psgProbe.onOff(false);
        scanRunner.acquireData();
        std::vector<double> fftPowerBackground = averageVectors(scanRunner.retrieveRawData());
        std::vector<double> freqAxis = scanRunner.retrieveRawAxis();
        scanRunner.flushData();

        std::cout << std::endl << "Saving data..." << std::endl;


        std::string fileName = "../../../plotting/visMeasurement/visData/" + std::to_string(1e3*(probe - xModeFreq)) + ".csv";
        saveVector(fftPowerProbeOn, fileName);

        fileName = "../../../plotting/visMeasurement/visData/bg_" + std::to_string(1e3*(probe - xModeFreq)) + ".csv";
        saveVector(fftPowerBackground, fileName);


        double vis, trueProbeFreq;
        std::tie(vis, trueProbeFreq) = getVisibility(fftPowerProbeOn, fftPowerBackground, probeFreqs, freqAxis, probe, xModeFreq);
        visibility.push_back(vis);
        trueProbeFreqs.push_back(trueProbeFreq);
    }
    printf("\n Acquisition complete!\n");

    saveVector(visibility, "../../../src/dataProcessing/visCurve.csv");
    saveVector(visibility, "../../../plotting/visMeasurement/visCurve.csv");
    saveVector(trueProbeFreqs, "../../../src/dataProcessing/trueProbeFreqs.csv");
    saveVector(trueProbeFreqs, "../../../plotting/visMeasurement/trueProbeFreqs.csv");


    // Cleanup
    psgProbe.onOff(false);

    _CrtDumpMemoryLeaks();
    return 0;
}



std::tuple<double, double> getVisibility(std::vector<double> fftPowerProbeOn, std::vector<double> fftPowerBackground, std::vector<double> probeFreqs, std::vector<double> freqAxis, double probeFreq, double yModeFreq) {
    std::cout << std::endl << "Getting visibility for probe at " << probeFreq << " GHz" << std::endl;
    
    double probeSeparation = (probeFreqs[1] - probeFreqs[0])*1e9;   // GHz -> Hz
    double freqSeparation = (freqAxis[1] - freqAxis[0])*1e6;        // MHz -> Hz

    int probeIndex = findClosestIndex(freqAxis, (probeFreq-yModeFreq)*1e3);
    int imageProbeIndex = findClosestIndex(freqAxis, (yModeFreq-probeFreq)*1e3);
    int searchWindow = 100;//(int)std::round(probeSeparation/freqSeparation)/2;

    std::vector<double> visibility(fftPowerProbeOn.size());
    for (int i = 0; i < fftPowerProbeOn.size(); ++i) {
        visibility[i] = (fftPowerProbeOn[i] - fftPowerBackground[i])/fftPowerBackground[i];
    }

    int alignedProbeIndex = findMaxIndex(visibility, max((probeIndex-searchWindow), 0), min((probeIndex+searchWindow),(int)visibility.size()));
    int alignedImageProbeIndex = findMaxIndex(visibility, max((imageProbeIndex-searchWindow), 0), min((imageProbeIndex+searchWindow),(int)visibility.size()));

    int sumWindow = 10;
    int selectedProbeIndex = visibility[alignedProbeIndex] > visibility[alignedImageProbeIndex] ? alignedProbeIndex : alignedImageProbeIndex;
    double finalVisibility = 0;
    for (int i = max((selectedProbeIndex-sumWindow/2), 0); i <= min((selectedProbeIndex+sumWindow/2),(int)fftPowerProbeOn.size()); ++i) {
        finalVisibility += visibility[i];
    }

    return std::make_tuple(finalVisibility, freqAxis[alignedProbeIndex]);
}