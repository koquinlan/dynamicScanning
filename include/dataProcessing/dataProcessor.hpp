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
    DataProcessor(){};
    ~DataProcessor(){};

    void displayState();

    void setFilterParams(double sampleRate, int poleNumber, double cutoffFrequency, double stopbandAttenuation);
    std::tuple<std::vector<double>, std::vector<double>, std::vector<double>> getFilterResponse();
    void displayFilterResponse();

    Spectrum loadSNR(std::string filenameSNR, std::string filenameSNRfreqs);

    std::vector<double> removeBadBins(std::vector<double> unfilteredRawSpectrum);

    void addRawSpectrumToRunningAverage(std::vector<double> rawSpectrum);
    void updateBaseline();

    std::vector<std::vector<double>> acquiredToRaw(fftw_complex* rawStream, int spectraPerAcquisition, int samplesPerSpectrum, fftw_plan plan);
    std::tuple<Spectrum, Spectrum> rawToProcessed(Spectrum rawSpectrum);
    Spectrum DataProcessor::processedToRescaled(Spectrum processedSpectrum);


    std::vector<int> badBins;

// private:
    int numSpectra=0;

    std::vector<double> runningAverage, currentBaseline;
    Spectrum SNR;

    std::mutex baselineMutex;
    std::mutex averageMutex;

    // Dsp::FilterDesign <class DesignClass, int Channels = 0, class StateType = DirectFormII>
    // DesignClass <int MaxOrder>
    Dsp::FilterDesign <Dsp::ChebyshevII::Design::LowPass<4>, 1> chebyshevFilter;

    double cutoffFrequency_, sampleRate_;
};


#endif // PROC_H