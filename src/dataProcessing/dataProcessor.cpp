/**
 * @file dataProcessor.cpp
 * @author Kyle Quinlan (kyle.quinlan@colorado.edu)
 * @brief Method definitions and documentation for the dataProcessor class. See include\dataProcessing\dataProcessor.hpp for the class definition.
 * @version 0.1
 * @date 2023-07-06
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include "decs.hpp"

void DataProcessor::displayState(){
    plt::figure();

    plt::plot(runningAverage);
    plt::plot(currentBaseline);

    plt::show();
}


void DataProcessor::setFilterParams(double sampleRate, int poleNumber, double cutoffFrequency, double stopbandAttenuation) {
    Dsp::Params params;
    params[0] = sampleRate;             // sample rate
    params[1] = poleNumber;             // pole number
    params[2] = cutoffFrequency;        // cutoff frequency
    params[3] = stopbandAttenuation;    // stopband attenuation

    chebyshevFilter.setParams(params);

    sampleRate_ = sampleRate;
    cutoffFrequency_ = cutoffFrequency;
}


/**
 * @brief 
 * 
 * @return std::tuple<std::vector<double>, std::vector<double>, std::vector<double>> 
 */
std::tuple<std::vector<double>, std::vector<double>, std::vector<double>> DataProcessor::getFilterResponse() {
    // Create an array of frequency points for the frequency response plot
    int numPoints = 10000; // You can adjust this based on the desired resolution
    std::vector<double> freqPoints(numPoints);
    std::vector<std::complex<double>> response(numPoints);

    std::vector<double> magnitude(numPoints);
    std::vector<double> phase(numPoints);

    for (int i = 0; i < numPoints; i++) {
        freqPoints[i] = 100 * static_cast<double>(i) / (numPoints - 1);
        
        response[i] = chebyshevFilter.response(freqPoints[i]*(cutoffFrequency_/sampleRate_));

        magnitude[i] = std::abs(response[i]);
        phase[i] = std::arg(response[i]);
    }

    // Undo phase wraps and write in units of PI
    unwrapPhase(phase);
    for (int i = 0; i < numPoints; i++) {
        phase[i] /= M_PI;
    }

    // Return response
    return std::make_tuple(freqPoints, magnitude, phase);
}


/**
 * @brief 
 * 
 */
void DataProcessor::displayFilterResponse() {
    std::vector<double> freqPoints, magnitude, phase, bidirectionalMagnitude;

    std::tie(freqPoints, magnitude, phase) = getFilterResponse();

    for (int i = 0; i < magnitude.size(); i++) {
        bidirectionalMagnitude.push_back(magnitude[i]*magnitude[i]);
    }

    // Create separate figures for magnitude and phase plots
    plt::figure();
    plt::loglog(freqPoints, magnitude);
    plt::loglog(freqPoints, bidirectionalMagnitude);
    plt::title("Magnitude Response");
    plt::xlabel("Normalized Frequency");
    plt::ylabel("Magnitude");
    plt::grid(true);


    plt::figure();
    plt::semilogx(freqPoints, phase);
    plt::title("Phase Response");
    plt::xlabel("Normalized Frequency");
    plt::ylabel("Phase (radians)");
    plt::grid(true);

    plt::show();
}


void DataProcessor::updateBaseline() {
    // Set up pointer to copy of running average to become the baseline
    currentBaseline = runningAverage;

    double* averagedData[1];
    averagedData[0] = currentBaseline.data();

    // Apply bidirectional filter to running average to extract the baseline
    chebyshevFilter.process((int) currentBaseline.size(), averagedData);

    std::reverse(currentBaseline.begin(), currentBaseline.end());
    chebyshevFilter.process((int) currentBaseline.size(), averagedData);
    std::reverse(currentBaseline.begin(), currentBaseline.end());
}


std::vector<double> DataProcessor::rawToIntermediate(std::vector<double> rawSpectrum) {
    std::vector<double> intermediateSpectrum(rawSpectrum.size());

    for (int i = 0; i < rawSpectrum.size(); i++) {
        intermediateSpectrum[i] = rawSpectrum[i]/currentBaseline[i];
    }

    return intermediateSpectrum;
}


std::vector<double> DataProcessor::intermediateToProcessed(std::vector<double> intermediateSpectrum) {
    std::vector<double> processedSpectrum(intermediateSpectrum.size());
    std::vector<double> processedBaseline = intermediateSpectrum;

    double* processedData[1];
    processedData[0] = processedBaseline.data();

    // Apply bidirectional filter to running average to extract the baseline
    chebyshevFilter.process((int) processedBaseline.size(), processedData);

    std::reverse(processedBaseline.begin(), processedBaseline.end());
    chebyshevFilter.process((int) processedBaseline.size(), processedData);
    std::reverse(processedBaseline.begin(), processedBaseline.end());

    for (int i = 0; i < intermediateSpectrum.size(); i++) {
        processedBaseline[i] = intermediateSpectrum[i]/processedBaseline[i];
    }

    return processedBaseline;
}


void DataProcessor::addRawSpectrumToRunningAverage(std::vector<double> rawSpectrum) {
    numSpectra++;

    if (numSpectra == 1) {
        runningAverage = rawSpectrum;
        return;
    }

    for (int i = 0; i < runningAverage.size(); i++) {
        runningAverage[i] = runningAverage[i]*(numSpectra-1)/numSpectra + rawSpectrum[i]/numSpectra;
    }
}


std::vector<double> DataProcessor::filterBadBins(std::vector<double> unfilteredRawSpectrum, double badBinThreshold) {
    std::vector<double> filteredSpectrum(unfilteredRawSpectrum.size());

    // Replace bad bins with NAN based on the current best available baseline
    for (int i = 0; i < unfilteredRawSpectrum.size(); i++) {
        filteredSpectrum[i] = unfilteredRawSpectrum[i]/currentBaseline[i] > badBinThreshold ? unfilteredRawSpectrum[i] : NAN;
    }

    return filteredSpectrum;
}