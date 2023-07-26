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


std::tuple<std::vector<double>, std::vector<double>, std::vector<double>> DataProcessor::setFilterParams(double sampleRate, int poleNumber, double cutoffFrequency, double stopbandAttenuation) {
    Dsp::Params params;
    params[0] = sampleRate;             // sample rate
    params[1] = poleNumber;             // pole number
    params[2] = cutoffFrequency;        // cutoff frequency
    params[3] = stopbandAttenuation;    // stopband attenuation

    chebyshevFilter.setParams(params);


    // Create an array of frequency points for the frequency response plot
    int numPoints = 10000; // You can adjust this based on the desired resolution
    std::vector<double> freqPoints(numPoints);
    std::vector<std::complex<double>> response(numPoints);

    std::vector<double> magnitude(numPoints);
    std::vector<double> phase(numPoints);

    for (int i = 0; i < numPoints; i++) {
        freqPoints[i] = 100 * static_cast<double>(i) / (numPoints - 1);
        
        response[i] = chebyshevFilter.response(freqPoints[i]*(cutoffFrequency/sampleRate));

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


std::vector<double> DataProcessor::rawToProcessed(std::vector<double> rawSpectrum) {
    return rawSpectrum;
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