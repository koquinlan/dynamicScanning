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


std::vector<double> DataProcessor::rawToProcessed(std::vector<double> rawSpectrum) {
    return rawSpectrum;
}

void DataProcessor::addRawSpectrumToBaseline(std::vector<double> rawSpectrum) {
    numSpectra++;

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