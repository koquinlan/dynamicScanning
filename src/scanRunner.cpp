/**
 * @file scanRunner.cpp
 * @author your name (you@domain.com)
 * @brief Method definitions and documentation for ScanRunner class. See include\scanRunner.hpp for class definition.
 * @version 0.1
 * @date 2023-08-10
 * 
 * @copyright Copyright (c) 2023
 * 
 * @note I could consider making this a parent class and having specific experiments inherit from this class. This would
 *       allow each experiment to define a different set of PSGs, or data acquisition method, for example.
 * 
 * @warning initAlazarCard() must be called before initFFTW() because the FFTW plan is dependent on the Alazar card's acquisition parameters.
 * 
 */


#include "decs.hpp"


/**
 * @brief Construct a new Scan Runner object. This constructor initializes the PSGs, Alazar card, FFTW, and DataProcessor.
 * 
 */
ScanRunner::ScanRunner() : alazarCard(1, 1),
                psgList{
                    PSG(27),  // PSG_DIFF
                    PSG(30),  // PSG_JPA
                    PSG(21)   // PSG_PROBE
                } {
    // Pumping parameters
    xModeFreq = 4.985; // GHz
    yModeFreq = 7.455396; // GHz

    diffPower = 6.73; //dBm
    jpaPower = 1.72; //dBm

    // Acquisition Parameters
    sampleRate = 32e6;
    RBW = 100;
    maxSpectraPerAcquisition = 250;

    // Filter Parameters
    cutoffFrequency = 10e3;
    poleNumber = 3;
    stopbandAttenuation = 15.0;


    // Set up member classes
    initPSGs();
    initAlazarCard();
    initFFTW();
    initProcessor();
}



/**
 * @brief Destroy the Scan Runner object. Ensure that any allocated memory is freed in this step, especially FFTW memory.
 * 
 */
ScanRunner::~ScanRunner() {
    // Turn off PSGs
    for (PSG psg : psgList) {
        psg.onOff(false);
    }

    // Free FFTW memory
    fftw_destroy_plan(fftwPlan);

    // Save FFTW wisdom
    fftw_export_wisdom_to_filename(wisdomFilePath);
}


/**
 * @brief Sets frequencies and powers for interaction PSGs and JPA pump. 
 * 
 */
void ScanRunner::initPSGs() {
    psgList[PSG_DIFF].setFreq(yModeFreq - xModeFreq);
    psgList[PSG_DIFF].setPow(diffPower);

    psgList[PSG_JPA].setFreq(xModeFreq * 2);
    psgList[PSG_JPA].setPow(jpaPower);
}


/**
 * @brief Calculates and sets acquisition parameters for Alazar card.
 * 
 */
void ScanRunner::initAlazarCard() {
    double samplesPerSpectrum = sampleRate/RBW;
    double samplesPerAcquisition = samplesPerSpectrum*maxSpectraPerAcquisition;

    alazarCard.setAcquisitionParameters((U32)sampleRate, (U32)samplesPerAcquisition, maxSpectraPerAcquisition);
    std::cout << "Acquisition parameters set. Collecting " << std::to_string(alazarCard.acquisitionParams.buffersPerAcquisition) << " buffers." << std::endl;
}


/**
 * @brief Initializes FFTW plan for data processing.
 * 
 * @warning Must be called after initAlazarCard() because the FFTW plan is dependent on the Alazar card's acquisition parameters.
 * 
 */
void ScanRunner::initFFTW() {
    // Try to import an FFTW plan if available
    wisdomFilePath = "fftw_wisdom.txt";
    if (fftw_import_wisdom_from_filename(wisdomFilePath) != 0) {
        std::cout << "Successfully imported FFTW wisdom from file." << std::endl;
    }
    else {
        std::cout << "Failed to import FFTW wisdom from file." << std::endl;
    }

    // Create an FFTW plan
    int N = (int)alazarCard.acquisitionParams.samplesPerBuffer;

    std::cout << "Creating plan for N = " << std::to_string(N) << std::endl;

    fftw_complex* fftwInput = reinterpret_cast<fftw_complex*>(fftw_malloc(sizeof(fftw_complex) * N));
    fftw_complex* fftwOutput = reinterpret_cast<fftw_complex*>(fftw_malloc(sizeof(fftw_complex) * N));
    fftwPlan = fftw_plan_dft_1d(N, fftwInput, fftwOutput, FFTW_FORWARD, FFTW_MEASURE);

    std::cout << "Plan created!" << std::endl;

    fftw_free(fftwInput);
    fftw_free(fftwOutput);
}


/**
 * @brief Initializes DataProcessor object. Initializes filter and loads SNR, bad bins and baseline if available.
 * 
 */
void ScanRunner::initProcessor() {
    // Create data processor
    dataProcessor.setFilterParams(alazarCard.acquisitionParams.sampleRate, poleNumber, cutoffFrequency, stopbandAttenuation);
    dataProcessor.loadSNR("../../../src/dataProcessing/visTheory.csv", "../../../src/dataProcessing/visTheoryFreqAxis.csv");


    // Try to import bad bins if available
    std::vector<double> badBins = readVector("badBins.csv");

    if (!badBins.empty()) {
        dataProcessor.badBins.reserve(badBins.size());
        std::transform(badBins.begin(), badBins.end(), std::back_inserter(dataProcessor.badBins), [](double d) { return static_cast<int>(d); }); // convert to int
    }
    else {
        std::cout << "Failed to import bad bins from file." << std::endl;
    }


    // Try to import baseline if available
    dataProcessor.currentBaseline = readVector("baseline.csv");
}


/**
 * @brief Runs a scan. This function assumes that the probes and frequency have been properly set and begins acquisition for a single data point.
 * 
 */
void ScanRunner::acquireData() {
    // Turn on PSGs
    psgList[PSG_DIFF].onOff(true);
    psgList[PSG_JPA].onOff(true);


    // Set up shared data
    SharedDataBasic sharedDataBasic;
    SharedDataProcessing sharedDataProc;
    SynchronizationFlags syncFlags;

    int N = (int)alazarCard.acquisitionParams.samplesPerBuffer;
    sharedDataBasic.samplesPerBuffer = alazarCard.acquisitionParams.samplesPerBuffer;


    // Begin the threads
    std::thread acquisitionThread(&ATS::AcquireDataMultithreadedContinuous, &alazarCard, std::ref(sharedDataBasic), std::ref(syncFlags));
    std::thread FFTThread(FFTThread, fftwPlan, N, std::ref(sharedDataBasic), std::ref(syncFlags));
    std::thread magnitudeThread(magnitudeThread, N, std::ref(sharedDataBasic), std::ref(sharedDataProc), std::ref(syncFlags), std::ref(dataProcessor));
    std::thread averagingThread(averagingThread, std::ref(sharedDataProc), std::ref(syncFlags), std::ref(dataProcessor), std::ref(yModeFreq));
    std::thread processingThread(processingThread, std::ref(sharedDataProc), std::ref(savedData), std::ref(syncFlags), std::ref(dataProcessor), std::ref(bayesFactors));
    std::thread decisionMakingThread(decisionMakingThread, std::ref(sharedDataProc), std::ref(syncFlags));


    // Wait for the threads to finish
    acquisitionThread.join();
    FFTThread.join();
    magnitudeThread.join();
    averagingThread.join();
    processingThread.join();
    decisionMakingThread.join();


    // Cleanup
    psgList[PSG_DIFF].onOff(false);
    psgList[PSG_JPA].onOff(false);

    reportPerformance();
}


/**
 * @brief Saves any data available to the scanRunner to csv files to be plotted later in python.
 * 
 */
void ScanRunner::saveData() {
    // Save the data
    std::vector<int> outliers = findOutliers(dataProcessor.runningAverage, 50, 4.5);

    std::vector<double> freq(alazarCard.acquisitionParams.samplesPerBuffer);
    for (int i = 0; i < freq.size(); ++i) {
        freq[i] = (static_cast<double>(i)-static_cast<double>(alazarCard.acquisitionParams.samplesPerBuffer)/2)
                    *alazarCard.acquisitionParams.sampleRate/alazarCard.acquisitionParams.samplesPerBuffer/1e6;
    }

    saveVector(freq, "../../../plotting/threadTests/freq.csv");
    saveVector(outliers, "../../../plotting/threadTests/outliers.csv");

    dataProcessor.updateBaseline();
    saveVector(dataProcessor.currentBaseline, "../../../plotting/threadTests/baseline.csv");
    saveVector(dataProcessor.currentBaseline, "baseline.csv");
    saveVector(dataProcessor.runningAverage, "../../../plotting/threadTests/runningAverage.csv");

    saveSpectrum(savedData.rawSpectra[0], "../../../plotting/threadTests/rawSpectrum.csv");
    saveSpectrum(savedData.processedSpectra[0], "../../../plotting/threadTests/processedSpectrum.csv");

    CombinedSpectrum combinedSpectrum;
    for (Spectrum rescaledSpectrum : savedData.rescaledSpectra){
        dataProcessor.addRescaledToCombined(rescaledSpectrum, combinedSpectrum);
    }
    saveCombinedSpectrum(combinedSpectrum, "../../../plotting/threadTests/combinedSpectrum.csv");

    saveSpectrum(bayesFactors.exclusionLine, "../../../plotting/threadTests/exclusionLine.csv");
}