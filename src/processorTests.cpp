/**
 * @file processorTests.cpp
 * @author Kyle Quinlan (kyle.quinlan@colorado.edu)
 * @brief 
 * @version 0.1
 * @date 2023-07-18
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include "decs.hpp"

int main() {
    // // Add some toy data to work with
    // std::string filename = "../../../src/dataProcessing/raw_data_probe_2.csv";
    // std::vector<std::vector<double>> toyRawData = readCSV(filename, 10);

    // std::cout << "Read " << toyRawData.size() << " spectra from " << filename << "\n" << std::endl;


    // Get ready for acquisition
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
    int spectraPerAcquisition = 64;

    int samplesPerSpectrum = (int)(sampleRate/RBW);
    int samplesPerAcquisition = samplesPerSpectrum*spectraPerAcquisition;

    // Set up PSGs
    printAvailableResources();

    PSG psg1_Diff(27);
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
    fftw_complex* rawStream = alazarCard.AcquireData();


    // Process data
    DataProcessor dataProcessor;
    double cutoffFrequency = 10e3;
    int poleNumber = 3;
    double stopbandAttenuation = 15.0;
    dataProcessor.setFilterParams(alazarCard.acquisitionParams.sampleRate, poleNumber, cutoffFrequency, stopbandAttenuation);

    dataProcessor.loadSNR("../../../src/dataProcessing/visTheory.csv", "../../../src/dataProcessing/visTheoryFreqAxis.csv");

    std::vector<std::vector<double>> rawData = dataProcessor.acquiredToRaw(rawStream, spectraPerAcquisition, samplesPerSpectrum, plan);
    fftw_free(rawStream);

    dataProcessor.badBins = findOutliers(averageVectors(rawData));
    for (int i = 0; i < rawData.size(); ++i) {
        rawData[i] = dataProcessor.removeBadBins(rawData[i]);
        dataProcessor.addRawSpectrumToRunningAverage(rawData[i]);
    }

    dataProcessor.updateBaseline();

    Spectrum rawSpectrum;
    rawSpectrum.powers = averageVectors(rawData);

    std::vector<double> freqAxis(samplesPerSpectrum);
    for (int i = 0; i < samplesPerSpectrum; ++i) {
        freqAxis[i] = (static_cast<double>(i)-static_cast<double>(samplesPerSpectrum)/2)*alazarCard.acquisitionParams.sampleRate/samplesPerSpectrum/1e6;
    }

    rawSpectrum.freqAxis = freqAxis;
    rawSpectrum.trueCenterFreq = yModeFreq;

    Spectrum processedSpectrum, processedBaseline;
    std::tie(processedSpectrum, processedBaseline) = dataProcessor.rawToProcessed(rawSpectrum);
    
    trimSpectrum(processedSpectrum, 0.1);
    trimSpectrum(processedBaseline, 0.1);
    dataProcessor.trimSNRtoMatch(processedSpectrum);

    Spectrum rescaledSpectrum = dataProcessor.processedToRescaled(processedSpectrum);

    CombinedSpectrum combinedSpectrum;
    dataProcessor.addRescaledToCombined(rescaledSpectrum, combinedSpectrum);


    // Save data for plotting
    saveVector(freqAxis, "../../../plotting/processorTests/freqAxis.csv");

    trimVector(freqAxis, 0.1);

    saveVector(freqAxis, "../../../plotting/processorTests/trimmedFreqAxis.csv");
    saveSpectrum(rawSpectrum, "../../../plotting/processorTests/rawSpectrum.csv");
    saveVector(dataProcessor.currentBaseline, "../../../plotting/processorTests/fullBaseline.csv");

    saveSpectrum(processedSpectrum, "../../../plotting/processorTests/processedSpectrum.csv");
    saveSpectrum(processedBaseline, "../../../plotting/processorTests/processedBaseline.csv");

    saveSpectrum(rescaledSpectrum, "../../../plotting/processorTests/rescaledSpectrum.csv");

    saveCombinedSpectrum(combinedSpectrum, "../../../plotting/processorTests/combinedSpectrum.csv");


    // Cleanup
    psg1_Diff.onOff(false);
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