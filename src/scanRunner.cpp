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
ScanRunner::ScanRunner(double maxIntegrationTime, int scanType) : alazarCard(1, 1),
                psgList{
                    PSG(27),  // PSG_DIFF
                    PSG(30),  // PSG_JPA
                    PSG(21)   // PSG_PROBE
                },
                scanType(scanType) {
    // Pumping parameters
    xModeFreq = 4.9871;    // GHz
    yModeFreq = 7.457296;  // GHz

    diffPower = 6.76; //dBm
    jpaPower = 2.66;  //dBm

    faxionFreq = yModeFreq; // GHz
    faxionPower = -35;      // dBm

    // Acquisition Parameters
    sampleRate = 32e6;
    RBW = 100;
    maxSpectraPerAcquisition = (int)(maxIntegrationTime*RBW);
    trueCenterFreq = yModeFreq*1e3 - 1; // Start 1 MHz below the y mode
    subSpectraAveragingNumber = 20;

    // Filter Parameters
    cutoffFrequency = 10e3;
    poleNumber = 3;
    stopbandAttenuation = 15.0;


    // Set up member classes
    initPSGs();
    initAlazarCard();
    initFFTW();
    initProcessor();
    initDecisionAgent();
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

    // Save FFTW wisdom
    fftw_export_wisdom_to_filename(wisdomFilePath);

    // Free FFTW memory
    fftw_destroy_plan(fftwPlan);
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

    if (scanType == SHARP_FAXION) {
        psgList[PSG_PROBE].setFreq(yModeFreq + faxionFreq - trueCenterFreq/1e3);
        psgList[PSG_PROBE].setPow(faxionPower);
    }
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
 * @brief 
 * 
 */
void ScanRunner::initDecisionAgent(){
    decisionAgent.SNR = dataProcessor.SNR;
}


/**
 * @brief Runs a scan. This function assumes that the probes and frequency have been properly set and begins acquisition for a single data point.
 * 
 */
void ScanRunner::acquireData() {
    // Turn on PSGs
    psgList[PSG_DIFF].onOff(true);
    psgList[PSG_JPA].onOff(true);

    if (scanType == SHARP_FAXION || scanType == BROAD_FAXION) {
        psgList[PSG_PROBE].onOff(true);
    }


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
    std::thread averagingThread(averagingThread, std::ref(sharedDataProc), std::ref(syncFlags), std::ref(dataProcessor), std::ref(trueCenterFreq), subSpectraAveragingNumber);
    std::thread processingThread(processingThread, std::ref(sharedDataProc), std::ref(savedData), std::ref(syncFlags), std::ref(dataProcessor), std::ref(bayesFactors));
    std::thread decisionMakingThread(decisionMakingThread, std::ref(sharedDataProc), std::ref(syncFlags), std::ref(bayesFactors), std::ref(decisionAgent));


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
    psgList[PSG_PROBE].onOff(false);

    reportPerformance();
}


/**
 * @brief Saves any data available to the scanRunner to csv files to be plotted later in python.
 * 
 */
void ScanRunner::saveData() {
    // Save the data
    std::vector<int> outliers = findOutliers(dataProcessor.runningAverage, 50, 4);

    std::vector<double> freq(alazarCard.acquisitionParams.samplesPerBuffer);
    for (int i = 0; i < freq.size(); ++i) {
        freq[i] = (static_cast<double>(i)-static_cast<double>(alazarCard.acquisitionParams.samplesPerBuffer)/2)
                    *alazarCard.acquisitionParams.sampleRate/alazarCard.acquisitionParams.samplesPerBuffer/1e6;
    }

    saveVector(freq, "../../../plotting/threadTests/freq.csv");
    saveVector(outliers, "../../../plotting/threadTests/outliers.csv");

    dataProcessor.updateBaseline();
    saveVector(dataProcessor.currentBaseline, "../../../plotting/threadTests/baseline.csv");
    saveVector(dataProcessor.runningAverage, "../../../plotting/threadTests/runningAverage.csv");

    saveSpectrum(savedData.rawSpectra[0], "../../../plotting/threadTests/rawSpectrum.csv");


    Spectrum processedSpectrum, foo;
    std::tie(processedSpectrum, foo) = dataProcessor.rawToProcessed(savedData.rawSpectra[0]);
    trimSpectrum(processedSpectrum, 0.1);
    saveSpectrum(processedSpectrum, "../../../plotting/threadTests/processedSpectrum.csv");


    dataProcessor.trimSNRtoMatch(processedSpectrum);
    Spectrum rescaledSpectrum = dataProcessor.processedToRescaled(processedSpectrum);


    saveCombinedSpectrum(savedData.combinedSpectrum, "../../../plotting/threadTests/combinedSpectrum.csv");
    saveSpectrum(bayesFactors.exclusionLine, "../../../plotting/threadTests/exclusionLine.csv");
}


void ScanRunner::refreshBaselineAndBadBins(int repeats, int subSpectra, int savePlots) {
    // Acquire some data
    acquireProcCalibration(repeats, subSpectra, savePlots);

    // Cleanup and saving
    saveVector(dataProcessor.currentBaseline, "baseline.csv");
    saveVector(dataProcessor.badBins, "badBins.csv");


    if (savePlots){
        std::vector<double> freq(alazarCard.acquisitionParams.samplesPerBuffer);
        for (int i = 0; i < freq.size(); ++i) {
            freq[i] = (static_cast<double>(i)-static_cast<double>(alazarCard.acquisitionParams.samplesPerBuffer)/2)
                        *alazarCard.acquisitionParams.sampleRate/alazarCard.acquisitionParams.samplesPerBuffer/1e6;
        }

        saveVector(freq, "../../../plotting/threadTests/freqTEST.csv");
        saveVector(dataProcessor.badBins, "../../../plotting/threadTests/outliersTEST.csv");

        saveVector(dataProcessor.currentBaseline, "../../../plotting/threadTests/baselineTEST.csv");
        saveVector(dataProcessor.runningAverage, "../../../plotting/threadTests/runningAverageTEST.csv");
    }

    dataProcessor.resetBaselining();
}


void ScanRunner::acquireProcCalibration(int repeats, int subSpectra, int savePlots) {
    // Turn on PSGs
    psgList[PSG_DIFF].onOff(true);
    psgList[PSG_JPA].onOff(true);


    // Acquire the data
    std::vector<std::vector<double>> fullRawData;

    for (int i = 0; i < repeats; i++){
        fftw_complex* rawStream = alazarCard.AcquireData();
        std::vector<std::vector<double>> rawData = dataProcessor.acquiredToRaw(rawStream, 
                                                                            alazarCard.acquisitionParams.buffersPerAcquisition, 
                                                                            alazarCard.acquisitionParams.samplesPerBuffer, 
                                                                            fftwPlan);
        fftw_free(rawStream);

        for (std::vector<double> data : rawData){
            fullRawData.push_back(data);
        }
    }

    std::vector<std::vector<double>> averagedRawData;
    for (int i=0; i < std::floor(fullRawData.size()/subSpectra); i++){
        std::vector<double> averagedSpectrum = averageVectors(std::vector<std::vector<double>>(fullRawData.begin()+i*subSpectra, fullRawData.begin()+(i+1)*subSpectra));
        averagedRawData.push_back(averagedSpectrum);
    }

    // Turn off PSGs
    psgList[PSG_DIFF].onOff(false);
    psgList[PSG_JPA].onOff(false);
    
    if(savePlots) {
        saveVector(averagedRawData[0], "../../../plotting/threadTests/rawDataTEST.csv");
    }


    // Get data ready to find badBins
    std::vector<double> freq(alazarCard.acquisitionParams.samplesPerBuffer);
    for (int i = 0; i < freq.size(); ++i) {
        freq[i] = (static_cast<double>(i)-static_cast<double>(alazarCard.acquisitionParams.samplesPerBuffer)/2)
                    *alazarCard.acquisitionParams.sampleRate/alazarCard.acquisitionParams.samplesPerBuffer/1e6;
    }

    dataProcessor.badBins = findOutliers(averageVectors(averagedRawData), 50, 5);
    for (int i = 0; i < averagedRawData.size(); ++i) {
        std::vector<double> cleanedRawData = dataProcessor.removeBadBins(averagedRawData[i]);
        dataProcessor.addRawSpectrumToRunningAverage(cleanedRawData);
    }

    dataProcessor.updateBaseline();
    

    // Do bad bin detection on processed spectra
    std::vector<std::vector<double>> processedSpectra(averagedRawData.size());

    for (int i = 0; i < averagedRawData.size(); ++i) {
        Spectrum rawSpectrum;
        rawSpectrum.powers = averagedRawData[i];
        rawSpectrum.freqAxis = freq;

        Spectrum processedSpectrum, foo;
        std::tie(processedSpectrum, foo) = dataProcessor.rawToProcessed(rawSpectrum);

        processedSpectra[i] = processedSpectrum.powers;
    }

    dataProcessor.badBins = findOutliers(averageVectors(processedSpectra), 100, 5);


    // Use the bad bins to find a clean baseline
    dataProcessor.resetBaselining();
    for (int i = 0; i < averagedRawData.size(); ++i) {
        std::vector<double> cleanedRawData = dataProcessor.removeBadBins(averagedRawData[i]);
        dataProcessor.addRawSpectrumToRunningAverage(cleanedRawData);
    }
    for (int bin : findOutliers(dataProcessor.runningAverage, 50, 4)){
        dataProcessor.badBins.push_back(bin);
    }

    dataProcessor.updateBaseline();
}


void ScanRunner::step(double stepSize) {
    bayesFactors.step(stepSize);

    trueCenterFreq += stepSize;
    psgList[PSG_PROBE].setFreq(yModeFreq + faxionFreq - trueCenterFreq/1e3);
}



void ScanRunner::setTarget(double targetCoupling) {
    decisionAgent.targetCoupling = targetCoupling;
}