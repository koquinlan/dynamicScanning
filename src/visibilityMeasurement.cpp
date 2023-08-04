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
    // Determine parameters for acquisition
    // Pumping parameters
    double xModeFreq = 4.98525;  // GHz
    double yModeFreq = 7.455646; // GHz

    double diffFreq = yModeFreq - xModeFreq;
    double jpaFreq = xModeFreq * 2;

    double diffPower = 6.73; //dBm
    double jpaPower = 12.96; //dBm

    // Acquisition parameters
    double sampleRate = 32e6;
    double RBW = 100;
    int spectraPerAcquisition = 32;

    int samplesPerSpectrum = (int)(sampleRate/RBW);
    int samplesPerAcquisition = samplesPerSpectrum*spectraPerAcquisition;

    // Probe parameters
    double probeSpan = 10; // MHz
    int numProbes = 200;

    std::vector<double> probeFreqs(numProbes);
    for (int i = 0; i < numProbes/2; ++i) {
        probeFreqs[2*i] = yModeFreq - (probeSpan/2*1e-3 - i*probeSpan/(numProbes-1)*1e-3);
        probeFreqs[2*i + 1] = yModeFreq + (probeSpan/2*1e-3 - i*probeSpan/(numProbes-1)*1e-3);
        std::cout << (probeFreqs[2*i] - yModeFreq)*1e3 << " " << (probeFreqs[2*i+1] - yModeFreq)*1e3 << " ";
    }
    std::cout << std::endl;

    // Set up PSGs
    printAvailableResources();

    PSG psg1_Diff(27);
    PSG psg3_Probe(21);
    PSG psg4_JPA(30);

    psg1_Diff.setFreq(diffFreq);
    psg1_Diff.setPow(diffPower);
    psg1_Diff.onOff(true);

    psg4_JPA.setFreq(jpaFreq);
    psg4_JPA.setPow(jpaPower);
    psg4_JPA.onOff(true);


    // Set up alazar card
    ATS alazarCard(1, 1);
    alazarCard.setAcquisitionParameters((U32)sampleRate, (U32)samplesPerAcquisition, spectraPerAcquisition);


    // Try to import an FFTW plan if available
    const char* wisdomFilePath = "fftw_wisdom.txt";
    if (fftw_import_wisdom_from_filename(wisdomFilePath) != 0) {
        std::cout << "Successfully imported FFTW wisdom from file." << std::endl;
    }
    else {
        std::cout << "Failed to import FFTW wisdom from file." << std::endl;
    }
    
    // Create an FFTW plan
    std::cout << "Creating plan for samplesPerSpectrum = " << std::to_string(samplesPerSpectrum) << std::endl;
    fftw_complex* fftwInput = reinterpret_cast<fftw_complex*>(fftw_malloc(sizeof(fftw_complex) * samplesPerSpectrum));
    fftw_complex* fftwOutput = reinterpret_cast<fftw_complex*>(fftw_malloc(sizeof(fftw_complex) * samplesPerSpectrum));
    fftw_plan plan = fftw_plan_dft_1d(samplesPerSpectrum, fftwInput, fftwOutput, FFTW_FORWARD, FFTW_ESTIMATE);
    std::cout << "Plan created!" << std::endl;


    // Acquire data
    std::vector<double> freqAxis(samplesPerSpectrum);
    for (int i = 0; i < samplesPerSpectrum; ++i) {
        freqAxis[i] = (static_cast<double>(i)-static_cast<double>(samplesPerSpectrum)/2)*alazarCard.acquisitionParams.sampleRate/samplesPerSpectrum/1e6;
    }
    saveVector(freqAxis, "../../../plotting/visFreq.csv");

    psg3_Probe.setPow(-60);

    std::vector<double> visibility, trueProbeFreqs;
    DataProcessor proc;
    for (double probe : probeFreqs) {
        printf("Taking data at %.6f GHz, %.2f %% complete\r", probe, trueProbeFreqs.size()/(double)numProbes*100);
        psg3_Probe.setFreq(probe);

        psg3_Probe.onOff(true);
        fftw_complex* rawProbeStream = alazarCard.AcquireData();
        std::vector<double> fftPowerProbeOn = averageVectors(proc.acquiredToRaw(rawProbeStream, spectraPerAcquisition, samplesPerSpectrum, plan));
        fftw_free(rawProbeStream);

        psg3_Probe.onOff(false);
        fftw_complex* rawBackgroundStream = alazarCard.AcquireData();
        std::vector<double> fftPowerBackground = averageVectors(proc.acquiredToRaw(rawBackgroundStream, spectraPerAcquisition, samplesPerSpectrum, plan));
        fftw_free(rawBackgroundStream);

        std::string fileName = "../../../plotting/visData/" + std::to_string(1e3*(probe - yModeFreq)) + ".csv";
        saveVector(fftPowerProbeOn, fileName);

        fileName = "../../../plotting/visData/bg_" + std::to_string(1e3*(probe - yModeFreq)) + ".csv";
        saveVector(fftPowerBackground, fileName);


        std::vector<int> outliers = findOutliers(fftPowerBackground);
        for (int outlier : outliers) {
            fftPowerBackground[outlier] = (fftPowerBackground[outlier-1] + fftPowerBackground[outlier+1])/2;
            fftPowerProbeOn[outlier] = (fftPowerProbeOn[outlier-1] + fftPowerProbeOn[outlier+1])/2;
        }

        double vis, trueProbeFreq;
        std::tie(vis, trueProbeFreq) = getVisibility(fftPowerProbeOn, fftPowerBackground, probeFreqs, freqAxis, probe, yModeFreq);
        visibility.push_back(vis);
        trueProbeFreqs.push_back(trueProbeFreq);

        fftPowerProbeOn.clear();
        std::vector<double>().swap(fftPowerProbeOn); // This line releases the memory
        fftPowerBackground.clear();
        std::vector<double>().swap(fftPowerBackground); // This line releases the memory
    }
    printf("\n Acqusiition complete!\n");

    saveVector(visibility, "../../../src/dataProcessing/visCurve.csv");
    saveVector(visibility, "../../../plotting/visCurve.csv");
    saveVector(trueProbeFreqs, "../../../src/dataProcessing/trueProbeFreqs.csv");
    saveVector(trueProbeFreqs, "../../../plotting/trueProbeFreqs.csv");


    // Cleanup
    psg1_Diff.onOff(false);
    psg3_Probe.onOff(false);
    psg4_JPA.onOff(false);

    fftw_export_wisdom_to_filename(wisdomFilePath);
    std::cout << "FFTW wisdom saved to file." << std::endl;

    fftw_destroy_plan(plan);
    fftw_free(fftwInput);
    fftw_free(fftwOutput);

    freqAxis.clear();
    std::vector<double>().swap(freqAxis);

    _CrtDumpMemoryLeaks();
    return 0;
}



std::tuple<double, double> getVisibility(std::vector<double> fftPowerProbeOn, std::vector<double> fftPowerBackground, std::vector<double> probeFreqs, std::vector<double> freqAxis, double probeFreq, double yModeFreq) {
    double probeSeparation = (probeFreqs[1] - probeFreqs[0])*1e9;   // GHz -> Hz
    double freqSeparation = (freqAxis[1] - freqAxis[0])*1e6;        // MHz -> Hz

    int probeIndex = findClosestIndex(freqAxis, (probeFreq-yModeFreq)*1e3);
    int imageProbeIndex = findClosestIndex(freqAxis, (2*yModeFreq-probeFreq)*1e3);
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