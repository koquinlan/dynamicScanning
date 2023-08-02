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

std::vector<double> processData(fftw_complex* rawStream, int spectraPerAcquisition, int samplesPerSpectrum, fftw_plan plan);

int main() {
    // Determine parameters for acquisition
    // Pumping parameters
    double xModeFreq = 4.9850755; // GHz
    double yModeFreq = 7.4554715; // GHz

    double diffFreq = yModeFreq - xModeFreq;
    double jpaFreq = xModeFreq * 2;

    double diffPower = 6.73; //dBm
    double jpaPower = 13.08; //dBm

    // Acquisition parameters
    double sampleRate = 32e6;
    double RBW = 100;
    int spectraPerAcquisition = 32;

    int samplesPerSpectrum = (int)(sampleRate/RBW);
    int samplesPerAcquisition = samplesPerSpectrum*spectraPerAcquisition;

    // Probe parameters
    double probeSpan = 10; // MHz
    int numProbes = 30;

    std::vector<double> probeFreqs(numProbes);
    for (int i = 0; i < numProbes; ++i) {
        probeFreqs[i] = yModeFreq - probeSpan/2*1e-3 + i*probeSpan/(numProbes-1)*1e-3;
        std::cout << probeFreqs[i] << " ";
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
    psg3_Probe.setPow(-60);

    for (double probe : probeFreqs) {
        printf("Taking data at %.3f MHz\n", probe*1e3);
        psg3_Probe.setFreq(probe);

        psg3_Probe.onOff(true);
        std::vector<double> fftPowerProbeOn = processData(alazarCard.AcquireData(), spectraPerAcquisition, samplesPerSpectrum, plan);
        psg3_Probe.onOff(false);
        std::vector<double> fftPowerBackground = processData(alazarCard.AcquireData(), spectraPerAcquisition, samplesPerSpectrum, plan);

        std::string fileName = "../../../plotting/visData/" + std::to_string(1e3*(probe - yModeFreq)) + ".csv";
        saveVector(fftPowerProbeOn, fileName);

        fileName = "../../../plotting/visData/bg_" + std::to_string(1e3*(probe - yModeFreq)) + ".csv";
        saveVector(fftPowerBackground, fileName);
    }


    // Cleanup
    psg1_Diff.onOff(false);
    psg3_Probe.onOff(false);
    psg4_JPA.onOff(false);

    fftw_export_wisdom_to_filename(wisdomFilePath);
    std::cout << "FFTW wisdom saved to file." << std::endl;

    fftw_destroy_plan(plan);
    fftw_free(fftwInput);
    fftw_free(fftwOutput);


    // Save the data
    std::vector<double> freq(samplesPerSpectrum);
    for (int i = 0; i < samplesPerSpectrum; ++i) {
        freq[i] = (static_cast<double>(i)-static_cast<double>(samplesPerSpectrum)/2)*alazarCard.acquisitionParams.sampleRate/samplesPerSpectrum/1e6;
    }

    saveVector(freq, "../../../plotting/visFreq.csv");

    return 0;
}


std::vector<double> processData(fftw_complex* rawStream, int spectraPerAcquisition, int samplesPerSpectrum, fftw_plan plan){
    // Slice the rawStream into subStreams and store them in the rawData vector
    std::vector<fftw_complex*> rawData(spectraPerAcquisition);
    std::vector<fftw_complex*> procData(spectraPerAcquisition);

    for (int i = 0; i < spectraPerAcquisition; ++i) {
        // Calculate the starting index for the current subStream
        int startIndex = i * samplesPerSpectrum;

        // Allocate memory for the subStream
        fftw_complex* subStream = (fftw_complex*) fftw_malloc(samplesPerSpectrum * sizeof(fftw_complex));

        // Copy data from rawStream to the subStream
        for (int j = 0; j < samplesPerSpectrum; ++j) {
            subStream[j][0] = rawStream[startIndex + j][0]; // Real part
            subStream[j][1] = rawStream[startIndex + j][1]; // Imaginary part
        }

        // Store the subStreams in the rawData and procData vectors
        rawData[i] = subStream;
        procData[i] = processDataFFT(subStream, plan, samplesPerSpectrum);
    }


    // Process the data into voltages
    std::vector<double> freq(samplesPerSpectrum);
    std::vector<std::vector<double>> fftVoltage(spectraPerAcquisition);
    std::vector<std::vector<double>> fftPower(spectraPerAcquisition);

    for (int i=0; i < spectraPerAcquisition; i++) {
        for (int j=0; j < samplesPerSpectrum; j++){
            fftVoltage[i].push_back( std::sqrt((procData[i][j][0]*procData[i][j][0] + procData[i][j][1]*procData[i][j][1]))/(double)samplesPerSpectrum );

            fftPower[i].push_back( fftVoltage[i][j]*fftVoltage[i][j]/50 ); // Hard code in 50 Ohm input impedance
        }
    }


    // Get the average
    std::vector<double> fftVoltageAvg(samplesPerSpectrum);
    std::vector<double> fftPowerAvg(samplesPerSpectrum);

    for (int i=0; i < samplesPerSpectrum; i++) {
        for (int j=0; j < spectraPerAcquisition; j++) {
            fftVoltageAvg[i] += fftVoltage[j][i];
            fftPowerAvg[i] += fftPower[j][i];
        }
        fftVoltageAvg[i] /= spectraPerAcquisition;
        fftPowerAvg[i] /= spectraPerAcquisition;
    }


    // Cleanup
    fftw_free(rawStream);
    for (fftw_complex* subStream : rawData) {
        fftw_free(subStream);
    }

    return fftPowerAvg;
}