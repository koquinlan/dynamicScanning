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
    // Add some toy data to work with
    std::string filename = "../../../src/dataProcessing/raw_data_probe_2.csv";
    std::vector<std::vector<double>> rawData = readCSV(filename, 10);

    std::cout << "Read " << rawData.size() << " spectra from " << filename << "\n" << std::endl;


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
    int spectraPerAcquisition = 32;

    int samplesPerSpectrum = (int)(sampleRate/RBW);
    int samplesPerAcquisition = samplesPerSpectrum*spectraPerAcquisition;

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

    DataProcessor proc;

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


    // Apply filtering to the running average
    proc.setFilterParams(30e6, 3, 20e3, 40.);
    proc.updateBaseline();

    std::vector<double> intermediateSpectrum = proc.rawToIntermediate(rawData[0]);
    std::vector<double> processedSpectrum = proc.intermediateToProcessed(intermediateSpectrum);

    for (int i=0; i<intermediateSpectrum.size(); i++) {
        intermediateSpectrum[i]--;
    }

    trimVector(intermediateSpectrum, 0.1);
    trimVector(processedSpectrum, 0.1);


    // Display results
    proc.displayFilterResponse();
    proc.displayState();

    plt::figure();
    plt::plot(rawData[0]);
    plt::plot(proc.runningAverage);
    plt::plot(proc.currentBaseline);


    plt::figure();
    // Plot intermediate and processed spectra
    plt::plot(intermediateSpectrum);
    plt::plot(processedSpectrum);
    plt::ylim(-1, 1);

    plt::show();

    return 0;
}