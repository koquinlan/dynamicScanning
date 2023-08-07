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
    // plt::figure();

    // plt::plot(runningAverage);
    // plt::plot(currentBaseline);

    // plt::show();
    return;
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

    // // Create separate figures for magnitude and phase plots
    // plt::figure();
    // plt::loglog(freqPoints, magnitude);
    // plt::loglog(freqPoints, bidirectionalMagnitude);
    // plt::title("Magnitude Response");
    // plt::xlabel("Normalized Frequency");
    // plt::ylabel("Magnitude");
    // plt::grid(true);


    // plt::figure();
    // plt::semilogx(freqPoints, phase);
    // plt::title("Phase Response");
    // plt::xlabel("Normalized Frequency");
    // plt::ylabel("Phase (radians)");
    // plt::grid(true);

    // plt::show();

    // plt::close();
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


std::tuple<Spectrum, Spectrum> DataProcessor::rawToProcessed(Spectrum rawSpectrum) {
    Spectrum intermediateSpectrum = rawSpectrum;

    for (int i = 0; i < rawSpectrum.powers.size(); i++) {
        intermediateSpectrum.powers[i] = rawSpectrum.powers[i]/currentBaseline[i];
    }


    // Set up containers for the baselining process
    Spectrum processedSpectrum = intermediateSpectrum;
    std::vector<double> processedBaseline = intermediateSpectrum.powers;

    double* processedBaselineData[1];
    processedBaselineData[0] = processedBaseline.data();


    // Apply bidirectional filter to running average to extract the baseline
    chebyshevFilter.process((int) processedBaseline.size(), processedBaselineData);

    std::reverse(processedBaseline.begin(), processedBaseline.end());
    chebyshevFilter.process((int) processedBaseline.size(), processedBaselineData);
    std::reverse(processedBaseline.begin(), processedBaseline.end());


    // Divide the intermediate spectrum by the processed baseline to get the processed spectrum
    for (int i = 0; i < intermediateSpectrum.powers.size(); i++) {
        processedSpectrum.powers[i] = intermediateSpectrum.powers[i]/processedBaseline[i]-1;
    }

    Spectrum processedBaselineSpectrum;
    processedBaselineSpectrum.powers = processedBaseline;
    processedBaselineSpectrum.freqAxis = processedSpectrum.freqAxis;

    return std::make_tuple(processedSpectrum, processedBaselineSpectrum);
}


void DataProcessor::addRawSpectrumToRunningAverage(std::vector<double> rawSpectrum) {
    numSpectra++;

    if (numSpectra == 1) {
        runningAverage = std::move(rawSpectrum);
        return;
    }

    // Optimized lambda function version of updating the running average
    const double factor = static_cast<double>(numSpectra - 1) / numSpectra;
    std::transform(runningAverage.begin(), runningAverage.end(), rawSpectrum.begin(), runningAverage.begin(),
                   [this, factor](double a, double b) { return a * factor + b / numSpectra; });
}



std::vector<double> DataProcessor::removeBadBins(std::vector<double> unfilteredRawSpectrum) {
    std::vector<double> filteredSpectrum = unfilteredRawSpectrum;

    // Replace bad bins with a linear fill
    for (int index : badBins) {
        double fillValue = (
            unfilteredRawSpectrum[(index + 1) % unfilteredRawSpectrum.size()] +
            unfilteredRawSpectrum[(index + unfilteredRawSpectrum.size() - 1) % unfilteredRawSpectrum.size()]
            ) / 2.0;
        
        filteredSpectrum[index] = fillValue;
    }

    return filteredSpectrum;
}



std::vector<std::vector<double>> DataProcessor::acquiredToRaw(fftw_complex* rawStream, int spectraPerAcquisition, int samplesPerSpectrum, fftw_plan plan){
    // Slice the rawStream into subStreams and store them in the rawData vector
    std::vector<fftw_complex*> procData(spectraPerAcquisition);

    for (int i = 0; i < spectraPerAcquisition; ++i) {
        int startIndex = i * samplesPerSpectrum;
        fftw_complex* subStream = (fftw_complex*) fftw_malloc(samplesPerSpectrum * sizeof(fftw_complex));

        for (int j = 0; j < samplesPerSpectrum; ++j) {
            subStream[j][0] = rawStream[startIndex + j][0]; // Real part
            subStream[j][1] = rawStream[startIndex + j][1]; // Imaginary part
        }

        procData[i] = processDataFFT(subStream, plan, samplesPerSpectrum);

        fftw_free(subStream);
    }


    // Process the data into voltages
    std::vector<std::vector<double>> fftVoltage(spectraPerAcquisition);
    std::vector<std::vector<double>> fftPower(spectraPerAcquisition);

    for (int i=0; i < spectraPerAcquisition; i++) {
        for (int j=0; j < samplesPerSpectrum; j++){
            fftVoltage[i].push_back( std::sqrt((procData[i][j][0]*procData[i][j][0] + procData[i][j][1]*procData[i][j][1]))/(double)samplesPerSpectrum );

            fftPower[i].push_back( fftVoltage[i][j]*fftVoltage[i][j]/50 ); // Hard code in 50 Ohm input impedance
        }
    }

    for (fftw_complex* data : procData) {
        fftw_free(data);
    }

    return fftPower;
}


Spectrum DataProcessor::loadSNR(std::string filenameSNR, std::string filenameSNRfreqs) {
    SNR.powers = readVector(filenameSNR);
    SNR.freqAxis = readVector(filenameSNRfreqs);

    return SNR;
}



Spectrum DataProcessor::processedToRescaled(Spectrum processedSpectrum) {
    Spectrum rescaledSpectrum = processedSpectrum;

    double mean, stddev;
    std::tie(mean, stddev) = vectorStats(rescaledSpectrum.powers);

    int startIndex=0;
    while (SNR.freqAxis[startIndex] < processedSpectrum.freqAxis[0]) {
        startIndex++;
    }
    for (int i=0; i < rescaledSpectrum.powers.size(); i++){
        rescaledSpectrum.powers[i] /= (stddev*SNR.powers[startIndex+i]);
    }
    
    return rescaledSpectrum;
}