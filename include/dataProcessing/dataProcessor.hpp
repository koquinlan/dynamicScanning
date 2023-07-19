/**
 * @file ATS.hpp
 * @author Kyle Quinlan (kyle.quinlan@colorado.edu)
 * @brief Class definition for data processor class. This should take in a raw spectrum and take it to where a Bayes factor is ready to be calculated.
 * @version 0.1
 * @date 2023-07-06
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#ifndef PROC_H
#define PROC_H

#include "decs.hpp"


class DataProcessor {
public:
    DataProcessor();
    ~DataProcessor();

    std::vector<double> rawToProcessed(std::vector<double> rawSpectrum);


    std::vector<double> filterBadBins(std::vector<double> unfilteredRawSpectrum);
    void addRawSpectrumToBaseline(std::vector<double> rawSpectrum);

private:
    int numSpectra;

    std::vector<double> currentBaseline;
};


#endif // PROC_H