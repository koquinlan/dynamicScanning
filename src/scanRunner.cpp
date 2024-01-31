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
ScanRunner::ScanRunner(double maxIntegrationTime, int scanType, int decisionMaking) : alazarCard(1, 1),
                psgList{
                    PSG(30),  // PSG_DIFF
                    PSG(21),  // PSG_JPA
                    PSG(27)   // PSG_PROBE
                },
                scanType(scanType) {
    // Pumping parameters
    xModeFreq = 5.208;    // GHz
    yModeFreq = 5.208;    // GHz

    diffPower = 6.76; //dBm
    jpaPower = 7.6;   //dBm

    faxionFreq = yModeFreq; // GHz
    faxionPower = -35;      // dBm

    // Acquisition Parameters
    sampleRate = 32e6;
    RBW = 100;
    maxSpectraPerAcquisition = (int)(maxIntegrationTime*RBW);
    trueCenterFreq = xModeFreq*1e3 - 1; // Start 1 MHz below the y mode
    subSpectraAveragingNumber = 20;

    // Filter Parameters
    cutoffFrequency = 10e3;
    poleNumber = 3;
    stopbandAttenuation = 15.0;

    // Data saving paths
    exclusionPath = "exclusionLineComparisons";
    savePath = "threadTests";


    // Set up member classes
    initPSGs();
    initAlazarCard();
    initFFTW();
    initProcessor();
    initDecisionAgent(decisionMaking);
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

    std::cout << "Trying to set acquisition parameters." << std::endl;
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
    dataProcessor.loadSNR("../../../src/dataProcessing/visSmoothed.csv", "../../../src/dataProcessing/visFreq.csv");


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
void ScanRunner::initDecisionAgent(int decisionMaking){
    decisionAgent.SNR = dataProcessor.SNR;

    if (!decisionMaking){ decisionAgent.toggleDecisionMaking(false); }
}


/**
 * @brief Runs a scan. This function assumes that the probes and frequency have been properly set and begins acquisition for a single data point.
 * 
 */
void ScanRunner::acquireData() {
    // Turn on PSGs
    // psgList[PSG_DIFF].onOff(true); // Temporarily turned off for cavity only operation
    psgList[PSG_JPA].onOff(true);

    if (scanType == SHARP_FAXION || scanType == BROAD_FAXION) {
        psgList[PSG_PROBE].onOff(true);
    }


    // Set up shared data
    SharedDataBasic sharedDataBasic;
    SharedDataProcessing sharedDataProc;
    SharedDataSaving sharedSavedData;
    SynchronizationFlags syncFlags;

    int N = (int)alazarCard.acquisitionParams.samplesPerBuffer;
    sharedDataBasic.samplesPerBuffer = alazarCard.acquisitionParams.samplesPerBuffer;

    // Begin the threads
    std::thread acquisitionThread(&ATS::AcquireDataMultithreadedContinuous, &alazarCard, std::ref(sharedDataBasic), std::ref(syncFlags));
    std::thread FFTThread(FFTThread, fftwPlan, N, std::ref(sharedDataBasic), std::ref(syncFlags));
    std::thread magnitudeThread(magnitudeThread, N, std::ref(sharedDataBasic), std::ref(sharedDataProc), std::ref(syncFlags), std::ref(dataProcessor));
    std::thread averagingThread(averagingThread, std::ref(sharedDataProc), std::ref(syncFlags), std::ref(dataProcessor), std::ref(trueCenterFreq), subSpectraAveragingNumber);
    std::thread processingThread(processingThread, std::ref(sharedDataProc), std::ref(savedData), std::ref(syncFlags), std::ref(dataProcessor), std::ref(bayesFactors));
    std::thread decisionMakingThread(decisionMakingThread, std::ref(sharedDataProc), std::ref(sharedSavedData), std::ref(syncFlags), std::ref(bayesFactors), std::ref(decisionAgent));
    #if SAVE_PROGRESS
    // std::thread savingThread(dataSavingThread, std::ref(sharedSavedData), std::ref(syncFlags));
    #endif

    // Wait for the threads to finish
    if (acquisitionThread.joinable()) { 
        acquisitionThread.join();
        std::cout << "Acquisition thread joined." << std::endl;
    } 
    else { std::cerr << "Acquisition thread is not joinable" << std::endl;}

    if (FFTThread.joinable()) { 
        FFTThread.join();
        std::cout << "FFT thread joined." << std::endl;
    } 
    else { std::cerr << "FFT thread is not joinable" << std::endl;}

    if (magnitudeThread.joinable()) { 
        magnitudeThread.join();
        std::cout << "Magnitude thread joined." << std::endl;
    } 
    else { std::cerr << "Magnitude thread is not joinable" << std::endl;}

    if (averagingThread.joinable()) { 
        averagingThread.join();
        std::cout << "Averaging thread joined." << std::endl;
    } 
    else { std::cerr << "Averaging thread is not joinable" << std::endl;}

    if(processingThread.joinable()) {
        processingThread.join();
        std::cout << "Processing thread joined." << std::endl;
        {
            std::lock_guard<std::mutex> lock(sharedDataProc.mutex);
            sharedDataProc.rescaledDataReadyCondition.notify_one();
        }
    } else {
        std::cerr << "Processing thread is not joinable" << std::endl;
    }

    if (decisionMakingThread.joinable()) { 
        decisionMakingThread.join();
        std::cout << "Decision thread joined." << std::endl;
    } 
    else { std::cerr << "Decision making thread is not joinable" << std::endl;}
    
    #if SAVE_PROGRESS
    // savingThread.join();

    std::string exclusionLineFilename = "../../../plotting/" + exclusionPath + "/scanProgress/exclusionLine_" + getDateTimeString() + ".csv";
    saveSpectraFromQueue(sharedSavedData.exclusionLineQueue, exclusionLineFilename);
    #endif


    // Cleanup
    psgList[PSG_DIFF].onOff(false);
    psgList[PSG_JPA].onOff(false);
    psgList[PSG_PROBE].onOff(false);

    reportPerformance();


    // Error recovery for when the threads don't finish properly
    if (syncFlags.errorFlag) {
        std::cout << "Error flag set. Recovering..." << std::endl;
    }
    else {
        std::cout << "Emptying backupDataQueue" << std::endl;
        while (!sharedDataBasic.backupDataQueue.empty()) {
            fftw_complex* data = sharedDataBasic.backupDataQueue.front();
            fftw_free(data);  // Free the memory
            sharedDataBasic.backupDataQueue.pop();
        }
    }
}



void ScanRunner::unrolledAcquisition() {
    // Turn on PSGs
    // psgList[PSG_DIFF].onOff(true); // Temporarily turned off for cavity only operation
    psgList[PSG_JPA].onOff(true);


    // Set up shared data
    SharedDataBasic sharedDataBasic;
    SharedDataProcessing sharedDataProc;
    SharedDataSaving sharedSavedData;
    SynchronizationFlags syncFlags;

    int N = (int)alazarCard.acquisitionParams.samplesPerBuffer;
    sharedDataBasic.samplesPerBuffer = alazarCard.acquisitionParams.samplesPerBuffer;


    // Begin the threads
    std::cout << "Launching acquisition thread." << std::endl;
    std::queue<fftw_complex*> backupDataQueue;
    std::thread acquisitionThread(&ATS::AcquireDataMultithreadedContinuous, &alazarCard, std::ref(sharedDataBasic), std::ref(syncFlags));
    acquisitionThread.join();

    std::cout << "Launching FFT thread." << std::endl;
    std::thread FFTThread(FFTThread, fftwPlan, N, std::ref(sharedDataBasic), std::ref(syncFlags));
    FFTThread.join();

    std::cout << "Launching magnitude thread." << std::endl;
    std::thread magnitudeThread(magnitudeThread, N, std::ref(sharedDataBasic), std::ref(sharedDataProc), std::ref(syncFlags), std::ref(dataProcessor));
    magnitudeThread.join();

    std::cout << "Launching averaging thread." << std::endl;
    std::thread averagingThread(averagingThread, std::ref(sharedDataProc), std::ref(syncFlags), std::ref(dataProcessor), std::ref(trueCenterFreq), subSpectraAveragingNumber);
    averagingThread.join();
    
    std::cout << "Launching processing thread." << std::endl;
    std::thread processingThread(processingThread, std::ref(sharedDataProc), std::ref(savedData), std::ref(syncFlags), std::ref(dataProcessor), std::ref(bayesFactors));
    processingThread.join();
    
    std::cout << "Launching decision making thread." << std::endl;
    std::thread decisionMakingThread(decisionMakingThread, std::ref(sharedDataProc), std::ref(sharedSavedData), std::ref(syncFlags), std::ref(bayesFactors), std::ref(decisionAgent));
    decisionMakingThread.join();


    // Cleanup
    psgList[PSG_DIFF].onOff(false);
    psgList[PSG_JPA].onOff(false);
    psgList[PSG_PROBE].onOff(false);
}



/**
 * @brief Saves any data available to the scanRunner to csv files to be plotted later in python.
 * 
 */
void ScanRunner::saveData(int dynamicFlag) {
    // Save the data
    std::vector<int> outliers = findOutliers(dataProcessor.runningAverage, 50, 4);

    std::vector<double> freq(alazarCard.acquisitionParams.samplesPerBuffer);
    for (int i = 0; i < freq.size(); ++i) {
        freq[i] = (static_cast<double>(i)-static_cast<double>(alazarCard.acquisitionParams.samplesPerBuffer)/2)
                    *alazarCard.acquisitionParams.sampleRate/alazarCard.acquisitionParams.samplesPerBuffer/1e6;
    }

    saveVector(freq, "../../../plotting/" + savePath + "/freq.csv");
    saveVector(outliers, "../../../plotting/" + savePath + "/outliers.csv");

    dataProcessor.updateBaseline();
    saveVector(dataProcessor.currentBaseline, "../../../plotting/" + savePath + "/baseline.csv");
    saveVector(dataProcessor.runningAverage, "../../../plotting/" + savePath + "/runningAverage.csv");

    saveSpectrum(savedData.rawSpectra[0], "../../../plotting/" + savePath + "/rawSpectrum.csv");


    Spectrum processedSpectrum, foo;
    std::tie(processedSpectrum, foo) = dataProcessor.rawToProcessed(savedData.rawSpectra[0]);
    trimSpectrum(processedSpectrum, 0.1);
    saveSpectrum(processedSpectrum, "../../../plotting/" + savePath + "/processedSpectrum.csv");


    dataProcessor.trimSNRtoMatch(processedSpectrum);
    Spectrum rescaledSpectrum = dataProcessor.processedToRescaled(processedSpectrum);


    saveCombinedSpectrum(savedData.combinedSpectrum, "../../../plotting/" + savePath + "/combinedSpectrum.csv");
    saveSpectrum(bayesFactors.exclusionLine, "../../../plotting/" + savePath + "/exclusionLine.csv");

    std::string exclusionLineFilename = "../../../plotting/" + exclusionPath + "/data/exclusionLine_";
    std::string scanInfoFilename = "../../../plotting/" + exclusionPath + "/metrics/scanInfo_";

    if (dynamicFlag){
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
    saveVector(dataProcessor.currentBaseline, "baseline.csv");
    saveVector(dataProcessor.badBins, "badBins.csv");


    if (savePlots){
        std::vector<double> freq(alazarCard.acquisitionParams.samplesPerBuffer);
        for (int i = 0; i < freq.size(); ++i) {
            freq[i] = (static_cast<double>(i)-static_cast<double>(alazarCard.acquisitionParams.samplesPerBuffer)/2)
                        *alazarCard.acquisitionParams.sampleRate/alazarCard.acquisitionParams.samplesPerBuffer/1e6;
        }

        saveVector(freq, "../../../plotting/baselineTests/baseline/freq.csv");
        saveVector(dataProcessor.badBins, "../../../plotting/baselineTests/baseline/outliers.csv");

        saveVector(dataProcessor.currentBaseline, "../../../plotting/baselineTests/baseline/baseline.csv");
        saveVector(dataProcessor.runningAverage, "../../../plotting/baselineTests/baseline/runningAverage.csv");
    }

    dataProcessor.resetBaselining();
}


void ScanRunner::acquireProcCalibration(int repeats, int subSpectra, int savePlots) {
    // Turn on PSGs
    // psgList[PSG_DIFF].onOff(true); // Temporarily turned off for cavity only operation
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
        saveVector(averagedRawData[0], "../../../plotting/baselineTests/baseline/rawData.csv");
    }


    // Get data ready to find badBins
    std::vector<double> freq(alazarCard.acquisitionParams.samplesPerBuffer);
    for (int i = 0; i < freq.size(); ++i) {
        freq[i] = (static_cast<double>(i)-static_cast<double>(alazarCard.acquisitionParams.samplesPerBuffer)/2)
                    *alazarCard.acquisitionParams.sampleRate/alazarCard.acquisitionParams.samplesPerBuffer/1e6;
    }

    dataProcessor.badBins = findOutliers(averageVectors(averagedRawData), 25, 5);
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

    dataProcessor.badBins = findOutliers(averageVectors(processedSpectra), 25, 5);


    // Use the bad bins to find a clean baseline
    dataProcessor.resetBaselining();
    for (int i = 0; i < averagedRawData.size(); ++i) {
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

    trueCenterFreq += stepSize;
    psgList[PSG_PROBE].setFreq(yModeFreq + faxionFreq - trueCenterFreq/1e3);
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