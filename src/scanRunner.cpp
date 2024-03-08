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
ScanRunner::ScanRunner(ScanParameters scanParams) : alazarCard(1, 1), scanParams(scanParams) {
    // Set up member classes
    initAlazarCard();
    initFFTW();
    initProcessor();
    initDecisionAgent(scanParams.topLevelParameters.decisionMaking);
}



/**
 * @brief Destroy the Scan Runner object. Ensure that any allocated memory is freed in this step, especially FFTW memory.
 * 
 */
ScanRunner::~ScanRunner() {
    // Save FFTW wisdom
    fftw_export_wisdom_to_filename((scanParams.topLevelParameters.wisdomPath + "fftw_wisdom.txt").c_str());

    // Free FFTW memory
    fftw_destroy_plan(fftwPlan);
}



/**
 * @brief Calculates and sets acquisition parameters for Alazar card.
 * 
 */
void ScanRunner::initAlazarCard() {
    int maxSpectraPerAcquisition = (int)(scanParams.dataParameters.maxIntegrationTime*scanParams.dataParameters.RBW);
    double samplesPerSpectrum = scanParams.dataParameters.sampleRate/scanParams.dataParameters.RBW;
    double samplesPerAcquisition = samplesPerSpectrum*maxSpectraPerAcquisition;

    std::cout << "Trying to set acquisition parameters." << std::endl;
    alazarCard.setAcquisitionParameters((U32)scanParams.dataParameters.sampleRate, (U32)samplesPerAcquisition, maxSpectraPerAcquisition);
    scanParams.dataParameters.sampleRate = alazarCard.acquisitionParams.sampleRate; // Update the sample rate in case it was changed
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
    if (fftw_import_wisdom_from_filename((scanParams.topLevelParameters.wisdomPath + "fftw_wisdom.txt").c_str()) != 0) {
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
    dataProcessor.setFilterParams(scanParams.dataParameters.sampleRate, 
                                  scanParams.filterParameters.poleNumber, 
                                  scanParams.filterParameters.cutoffFrequency, 
                                  scanParams.filterParameters.stopbandAttenuation);
    dataProcessor.loadSNR(scanParams.topLevelParameters.visPath + "visSmoothed.csv", scanParams.topLevelParameters.visPath + "visFreq.csv");


    // Try to import bad bins if available
    std::vector<double> badBins = readVector(scanParams.topLevelParameters.baselinePath + "badBins.csv");

    if (!badBins.empty()) {
        dataProcessor.badBins.reserve(badBins.size());
        std::transform(badBins.begin(), badBins.end(), std::back_inserter(dataProcessor.badBins), [](double d) { return static_cast<int>(d); }); // convert to int
    }
    else {
        std::cout << "Failed to import bad bins from file." << std::endl;
    }


    // Try to import baseline if available
    dataProcessor.currentBaseline = readVector(scanParams.topLevelParameters.baselinePath + "baseline.csv");
}



/**
 * @brief 
 * 
 */
void ScanRunner::initDecisionAgent(int decisionMaking){
    decisionAgent.SNR = dataProcessor.SNR;

    if (!decisionMaking){ decisionAgent.toggleDecisionMaking(false); }
}


/**
 * freqRes - frequency resolution in MHz
 * startIndex - index of the frequency axis where the exclusion line starts
 * cutoffIndex - index of the frequency axis where the exclusion line is cut off
 * previousCenterFreq - the center frequency of the previous spectrum
 * 
 * exclusionLine - Spectrum object containing the exclusion line
 * coeffSumA - vector of coefficients in the quadratic formula for the 90% excluded coupling strength
 * coeffSumB - vector of coefficients in the quadratic formula for the 90% excluded coupling strength
 * 
 * steps:
 *  1. Load the exclusionLine, coeffSumA, and coeffSumB from the previous scan
 *  2. Load the freqRes, statIndex, and cutoffIndex from the previous scan using the json file
 *  3. Determine the step size via previousCenterFreq - newCenterFreq
 *  4. Call the step function with the step size
 */
void ScanRunner::loadStateAndStep() {
    // Load the exclusion line, coeffSumA, and coeffSumB from the previous scan
    bayesFactors.exclusionLine = readSpectrum(scanParams.topLevelParameters.statePath + "exclusionLine.csv");
    bayesFactors.coeffSumA = readVector(scanParams.topLevelParameters.statePath + "coeffSumA.csv");
    bayesFactors.coeffSumB = readVector(scanParams.topLevelParameters.statePath + "coeffSumB.csv");

    // Load the freqRes, statIndex, and cutoffIndex from the previous scan using the json file
    std::string jsonFilename = scanParams.topLevelParameters.statePath + "scanInfo.json";
    std::ifstream jsonFile(jsonFilename);
    json scanInfo;
    jsonFile >> scanInfo;

    bayesFactors.freqRes = scanInfo["freqRes"];
    bayesFactors.startIndex = scanInfo["startIndex"];
    bayesFactors.cutoffIndex = scanInfo["cutoffIndex"];

    // Call the step function with the step size
    if (scanParams.dataParameters.stepSize != 0){
        bayesFactors.step(scanParams.dataParameters.stepSize);
    }   
}

void ScanRunner::saveState() {
    // Save the exclusion line, coeffSumA, and coeffSumB
    saveSpectrum(bayesFactors.exclusionLine, scanParams.topLevelParameters.statePath + "exclusionLine.csv");
    saveVector(bayesFactors.coeffSumA, scanParams.topLevelParameters.statePath + "coeffSumA.csv");
    saveVector(bayesFactors.coeffSumB, scanParams.topLevelParameters.statePath + "coeffSumB.csv");

    // Save the freqRes, statIndex, and cutoffIndex to a json file
    json scanInfo;
    scanInfo["freqRes"] = bayesFactors.freqRes;
    scanInfo["startIndex"] = bayesFactors.startIndex;
    scanInfo["cutoffIndex"] = bayesFactors.cutoffIndex;
    scanInfo["previousCenterFreq"] = scanParams.dataParameters.trueCenterFreq;

    std::ofstream jsonFile(scanParams.topLevelParameters.statePath + "scanInfo.json");
    jsonFile << scanInfo;
}


/**
 * @brief Runs a scan. This function assumes that the probes and frequency have been properly set and begins acquisition for a single data point.
 * 
 */
void ScanRunner::acquireData() {
    int N = (int)alazarCard.acquisitionParams.samplesPerBuffer;

    // Set up shared data
    ThreadSafeQueue<fftw_complex*> rawQueue;
    ThreadSafeQueue<fftw_complex*> fftQueue;
    ThreadSafeQueue<std::vector<double>> magQueue;
    ThreadSafeQueue<Spectrum> procQueue;
    ThreadSafeQueue<CombinedSpectrum> decisionQueue;

    std::atomic<bool> triggerEnd(false);


    // Begin the threads
    std::thread acquisitionThread(&ATS::AcquireDataMultithreadedContinuous, &alazarCard, std::ref(rawQueue), std::ref(triggerEnd));
    std::thread fftThread(fftThread, fftwPlan, N, std::ref(rawQueue), std::ref(fftQueue));
    std::thread magnitudeThread(magnitudeThread, N, std::ref(dataProcessor), std::ref(fftQueue), std::ref(magQueue));
    std::thread averagingThread(averagingThread, std::ref(dataProcessor), std::ref(scanParams.dataParameters.trueCenterFreq), std::ref(magQueue), std::ref(procQueue), scanParams.dataParameters.subSpectraAveragingNumber);
    std::thread processingThread(processingThread, std::ref(dataProcessor), std::ref(procQueue), std::ref(decisionQueue));
    std::thread decisionMakingThread(decisionMakingThread, std::ref(bayesFactors), std::ref(decisionAgent), std::ref(decisionQueue), std::ref(triggerEnd));


    // Wait for the threads to finish
    acquisitionThread.join();
    fftThread.join();
    magnitudeThread.join();
    averagingThread.join();
    processingThread.join();
    decisionMakingThread.join();


    // Cleanup
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
    for (std::size_t i = 0; i < freq.size(); ++i) {
        freq[i] = (static_cast<double>(i)-static_cast<double>(alazarCard.acquisitionParams.samplesPerBuffer)/2)
                    *scanParams.dataParameters.sampleRate/alazarCard.acquisitionParams.samplesPerBuffer/1e6;
    }

    saveVector(freq, scanParams.topLevelParameters.savePath + "freq.csv");
    saveVector(outliers, scanParams.topLevelParameters.savePath + "outliers.csv");

    dataProcessor.updateBaseline();
    saveVector(dataProcessor.currentBaseline, scanParams.topLevelParameters.savePath + "baseline.csv");
    saveVector(dataProcessor.runningAverage, scanParams.topLevelParameters.savePath + "runningAverage.csv");

    // saveSpectrum(savedData.rawSpectra[0], scanParams.topLevelParameters.savePath + "rawSpectrum.csv");


    // Spectrum processedSpectrum, foo;
    // std::tie(processedSpectrum, foo) = dataProcessor.rawToProcessed(savedData.rawSpectra[0]);
    // trimSpectrum(processedSpectrum, 0.1);
    // saveSpectrum(processedSpectrum, scanParams.topLevelParameters.savePath + "processedSpectrum.csv");


    // dataProcessor.trimSNRtoMatch(processedSpectrum);
    // Spectrum rescaledSpectrum = dataProcessor.processedToRescaled(processedSpectrum);


    // saveCombinedSpectrum(savedData.combinedSpectrum, scanParams.topLevelParameters.savePath + "combinedSpectrum.csv");
    saveSpectrum(bayesFactors.exclusionLine, scanParams.topLevelParameters.savePath + "exclusionLine.csv");

    std::string exclusionLineFilename = scanParams.topLevelParameters.savePath + "data/exclusionLine_";
    std::string scanInfoFilename = scanParams.topLevelParameters.savePath + "metrics/scanInfo_";

    if (scanParams.topLevelParameters.decisionMaking){
        exclusionLineFilename += "dynamic_";
        scanInfoFilename += "dynamic_";
    }

    exclusionLineFilename += getDateTimeString() + ".csv";
    scanInfoFilename += getDateTimeString() + ".csv";

    saveSpectrum(bayesFactors.exclusionLine, exclusionLineFilename);
    saveVector(getMetric(ACQUIRED_SPECTRA), scanInfoFilename);
    // Add in scan info saving
}


void ScanRunner::refreshBaselineAndBadBins(int repeats, int subSpectra, int savePlots) {
    // Acquire some data
    acquireProcCalibration(repeats, subSpectra, savePlots);

    // Cleanup and saving
    saveVector(dataProcessor.currentBaseline, scanParams.topLevelParameters.baselinePath + "baseline.csv");
    saveVector(dataProcessor.badBins, scanParams.topLevelParameters.baselinePath + "badBins.csv");


    if (savePlots){
        std::vector<double> freq(alazarCard.acquisitionParams.samplesPerBuffer);
        for (std::size_t i = 0; i < freq.size(); ++i) {
            freq[i] = (static_cast<double>(i)-static_cast<double>(alazarCard.acquisitionParams.samplesPerBuffer)/2)
                        *scanParams.dataParameters.sampleRate/alazarCard.acquisitionParams.samplesPerBuffer/1e6;
        }

        saveVector(freq, scanParams.topLevelParameters.baselinePath + "fullPlots/freq.csv");
        saveVector(dataProcessor.badBins, scanParams.topLevelParameters.baselinePath + "fullPlots/outliers.csv");

        saveVector(dataProcessor.currentBaseline, scanParams.topLevelParameters.baselinePath + "fullPlots/baseline.csv");
        saveVector(dataProcessor.runningAverage, scanParams.topLevelParameters.baselinePath + "fullPlots/runningAverage.csv");
    }

    dataProcessor.resetBaselining();
}


void ScanRunner::acquireProcCalibration(int repeats, int subSpectra, int savePlots) {
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
    
    if(savePlots) {
        saveVector(averagedRawData[0], scanParams.topLevelParameters.baselinePath + "fullPlots/rawData.csv");
    }


    // Get data ready to find badBins
    std::vector<double> freq(alazarCard.acquisitionParams.samplesPerBuffer);
    for (std::size_t i = 0; i < freq.size(); ++i) {
        freq[i] = (static_cast<double>(i)-static_cast<double>(alazarCard.acquisitionParams.samplesPerBuffer)/2)
                    *scanParams.dataParameters.sampleRate/alazarCard.acquisitionParams.samplesPerBuffer/1e6;
    }

    dataProcessor.badBins = findOutliers(averageVectors(averagedRawData), 25, 5);
    for (std::size_t i = 0; i < averagedRawData.size(); ++i) {
        std::vector<double> cleanedRawData = dataProcessor.removeBadBins(averagedRawData[i]);
        dataProcessor.addRawSpectrumToRunningAverage(cleanedRawData);
    }

    dataProcessor.updateBaseline();
    

    // Do bad bin detection on processed spectra
    std::vector<std::vector<double>> processedSpectra(averagedRawData.size());

    for (std::size_t i = 0; i < averagedRawData.size(); ++i) {
        Spectrum rawSpectrum;
        rawSpectrum.powers = averagedRawData[i];
        rawSpectrum.freqAxis = freq;

        Spectrum processedSpectrum, foo;
        std::tie(processedSpectrum, foo) = dataProcessor.rawToProcessed(rawSpectrum);

        processedSpectra[i] = processedSpectrum.powers;
    }

    dataProcessor.badBins = findOutliers(averageVectors(processedSpectra), 25, 5);


    // Use the bad bins to find a clean baseline
    dataProcessor.resetBaselining();
    for (std::size_t i = 0; i < averagedRawData.size(); ++i) {
        std::vector<double> cleanedRawData = dataProcessor.trimDC(dataProcessor.removeBadBins(averagedRawData[i]));
        dataProcessor.addRawSpectrumToRunningAverage(cleanedRawData);
    }
    for (int bin : findOutliers(dataProcessor.runningAverage, 50, 4)){
        dataProcessor.badBins.push_back(bin);
    }

    dataProcessor.updateBaseline();
}


void ScanRunner::step(double stepSize) {
    bayesFactors.step(stepSize);

    scanParams.dataParameters.trueCenterFreq += stepSize;
}



void ScanRunner::setTarget(double targetCoupling) {
    decisionAgent.targetCoupling = targetCoupling;
}



std::vector<std::vector<double>> ScanRunner::retrieveRawData() {
    std::vector<std::vector<double>> rawData;

    for (Spectrum spectrum : savedData.rawSpectra){
        rawData.push_back(spectrum.powers);
    }
    
    return rawData;
}



std::vector<double> ScanRunner::retrieveRawAxis() {
    return savedData.rawSpectra[0].freqAxis;
}



void ScanRunner::flushData() {
    savedData.rawSpectra.clear();
    savedData.processedSpectra.clear();
    savedData.rescaledSpectra.clear();
    savedData.combinedSpectrum.powers.clear();
    savedData.combinedSpectrum.freqAxis.clear();

    bayesFactors.exclusionLine.powers.clear();
    bayesFactors.exclusionLine.freqAxis.clear();
    bayesFactors.coeffSumA.clear();
    bayesFactors.coeffSumB.clear();

    dataProcessor.resetBaselining();
    dataProcessor.currentBaseline = readVector("baseline.csv");
}