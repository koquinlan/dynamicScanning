/**
 * @file multiThreading.cpp
 * @author Kyle Quinlan (kyle.quinlan@colorado.edu)
 * @brief Provides function definitions meant for use in multithreaded data acquisition. See ATS::AcquireDataMultithreadedContinuous for further usage.
 * @version 0.1
 * @date 2023-06-30
 * 
 * @copyright Copyright (c) 2023
 * 
 * @note See include/ATS.hpp for shared data and sync flag structures
 * 
 * @todo Change file I/O system to HDF5 or similar
 * @todo Implement real decision making engine in decisionMakingThread
 * 
 */

#include "decs.hpp"

/**
 * @brief Function to be run in a separate thread in parallel with ATS::AcquireDataMultithreadedContinuous. Fourier transforms incoming data and
 *        pushes the result to the decision making and saving queues.
 * 
 * @param plan - FFTW plan object for the FFT
 * @param samplesPerSpectrum - Number of samples per buffer shared in the data queue
 * @param sharedData - Struct containing data shared between threads
 * @param syncFlags - Struct containing synchronization flags shared between threads
 */
void FFTThread(fftw_plan plan, int samplesPerSpectrum, SharedDataBasic& sharedData, SynchronizationFlags& syncFlags) {
    try{
    int numProcessed = 0;
    while (true) {
        // std::cout << "Waiting for data..." << std::endl;

        // Wait for signal from dataReadyCondition or immediately continue if the data queue is not empty (lock releases while waiting)
        std::unique_lock<std::mutex> lock(sharedData.mutex);
        sharedData.dataReadyCondition.wait(lock, [&sharedData]() {
            return !sharedData.dataQueue.empty();
        });


        // Process data until data queue is empty (lock is reaquired before checking the data queue)
        startTimer(TIMER_FFT);
        while (!sharedData.dataQueue.empty()) {
            // Get the pointer to the data from the queue
            fftw_complex* complexOutput = sharedData.dataQueue.front();
            sharedData.dataQueue.pop();
            lock.unlock();

            // Process the data (lock is released while processing)
            fftw_complex* FFTData = processDataFFT(complexOutput, plan, samplesPerSpectrum);
            numProcessed++;


            // Acquire a new lock_guard and push the processed data to the shared queue
            {
                std::lock_guard<std::mutex> lock(sharedData.mutex);
                fftw_free(complexOutput);  // Free the memory allocated for the raw data
                sharedData.FFTDataQueue.push(FFTData);
            }
            sharedData.FFTDataReadyCondition.notify_one();
            sharedData.saveReadyCondition.notify_one();
            
            lock.lock();  // Reacquire lock before checking the data queue
        }
        stopTimer(TIMER_FFT);

        // Check if the acquisition and processing is complete or another thread threw an error
        {
            std::lock_guard<std::mutex> lock(syncFlags.mutex);
            if (syncFlags.acquisitionComplete && sharedData.dataQueue.empty()) {
                std::cout << "FFT thread exiting. Processed " << std::to_string(numProcessed) << " spectra." << std::endl;

                syncFlags.FFTComplete = true;
                sharedData.FFTDataReadyCondition.notify_one();
                break;  // Exit the processing thread
            }

            if (syncFlags.errorFlag) {
                std::cout << "FFT thread gracefully exiting due to error." << std::endl;

                sharedData.FFTDataReadyCondition.notify_one();
                break;
            }
        }
    }
    }
    catch (const std::exception& e) {
        std::lock_guard<std::mutex> lock(syncFlags.mutex);
        syncFlags.errorFlag = true;
        syncFlags.errorMessage = "FFTThread: " + std::string(e.what());
        std::cout << syncFlags.errorMessage << '\n';
    }
}


void magnitudeThread(int samplesPerSpectrum, SharedDataBasic& sharedData, SharedDataProcessing& sharedDataProc, SynchronizationFlags& syncFlags, DataProcessor& dataProcessor) {
    try
    {
        int numProcessed = 0;
        while (true) {
            // std::cout << "Waiting for data..." << std::endl;

            // Wait for signal from dataReadyCondition or immediately continue if the data queue is not empty (lock releases while waiting)
            std::unique_lock<std::mutex> lock(sharedData.mutex);
            sharedData.FFTDataReadyCondition.wait(lock, [&sharedData]() {
                return !sharedData.FFTDataQueue.empty();
            });


            // Process data until data queue is empty (lock is reaquired before checking the data queue)
            startTimer(TIMER_MAG);
            while (!sharedData.FFTDataQueue.empty()) {

                // Get the pointer to the data from the queue
                fftw_complex* FFTData = sharedData.FFTDataQueue.front();
                sharedData.FFTDataQueue.pop();
                lock.unlock();

                std::vector<double> magData(samplesPerSpectrum);
                for (int i = 0; i < samplesPerSpectrum; i++) {
                    magData[i] = ( FFTData[i][0]*FFTData[i][0] + FFTData[i][1]*FFTData[i][1] ) / samplesPerSpectrum / 50; // Hard code in 50 Ohm input impedance
                }

                magData = dataProcessor.trimDC(dataProcessor.removeBadBins(magData));

                if (magData.empty()) {
                    std::cout << "Error: Second unexpected empty data in magnitude thread." << std::endl;
                    std::cout << "Expected magData.size() = " << std::to_string(samplesPerSpectrum) << std::endl;
                }

                // Free the memory allocated for the fft data
                fftw_free(FFTData);
                numProcessed++;


                // Acquire a new lock_guard and push the processed data to the shared queue
                {
                    std::lock_guard<std::mutex> lock(sharedData.mutex);
                    if (!magData.empty()) {
                        sharedDataProc.magDataQueue.push(magData);
                    }
                }
                sharedDataProc.magDataReadyCondition.notify_one();
                
                lock.lock();  // Reacquire lock before checking the data queue
            }
            stopTimer(TIMER_MAG);

            // Check if the acquisition and processing is complete
            {
                std::lock_guard<std::mutex> lock(syncFlags.mutex);
                if (syncFlags.FFTComplete && sharedData.FFTDataQueue.empty()) {
                    std::cout << "Magnitude thread exiting. Processed " << std::to_string(numProcessed) << " spectra." << std::endl;

                    syncFlags.magnitudeComplete = true;
                    sharedDataProc.magDataReadyCondition.notify_one();
                    break;  // Exit the processing thread
                }

                if (syncFlags.errorFlag) {
                    std::cout << "Magnitude thread gracefully exiting due to error." << std::endl;

                    sharedDataProc.magDataReadyCondition.notify_one();
                    break;
                }
            }
        }
    }
    catch (const std::exception& e) {
        std::lock_guard<std::mutex> lock(syncFlags.mutex);
        syncFlags.errorFlag = true;
        syncFlags.errorMessage = "MagnitudeThread: " + std::string(e.what());
        std::cout << syncFlags.errorMessage << '\n';
    }
}



void averagingThread(SharedDataProcessing& sharedData, SynchronizationFlags& syncFlags, DataProcessor& dataProcessor, double trueCenterFreq, int subSpectraAveragingNumber = 20) {
    try{
    int subSpectraAveraged = 0;
    int totalProcessed = 0;
    while (true) {
        // Wait for signal from dataReadyCondition or immediately continue if the data queue is not empty (lock releases while waiting)
        std::unique_lock<std::mutex> lock(sharedData.mutex);
        sharedData.magDataReadyCondition.wait(lock, [&sharedData, &subSpectraAveragingNumber, &syncFlags]() {
            return (sharedData.magDataQueue.size() >= subSpectraAveragingNumber) || syncFlags.magnitudeComplete;
        });

        startTimer(TIMER_AVERAGE);

        int procNumber = min(subSpectraAveragingNumber, (int) sharedData.magDataQueue.size());
        std::vector<std::vector<double>> subSpectra;

        int preProcQueueSize = (int) sharedData.magDataQueue.size();
        for(int i = 0; i < procNumber; i++) {
            // Get the pointer to the data from the queue
            if (sharedData.magDataQueue.front().empty()) {
                std::cout << "Error: Unexpected empty data in averaging thread." << std::endl;
                std::cout << "Expected procNumber = " << std::to_string(procNumber) << std::endl;
                std::cout << "Original sharedData.magDataQueue.size() = " << std::to_string(preProcQueueSize) << std::endl;
                std::cout << "Spectrum Number = " << std::to_string(i) << std::endl;
            }
            else{
                subSpectra.push_back(sharedData.magDataQueue.front());
                // Update the running average using DataProcessor
                dataProcessor.addRawSpectrumToRunningAverage(sharedData.magDataQueue.front());
            }
            sharedData.magDataQueue.pop();
        }
        lock.unlock();

        Spectrum rawSpectrum;
        rawSpectrum.powers = averageVectors(subSpectra);
        rawSpectrum.freqAxis = dataProcessor.SNR.freqAxis;
        rawSpectrum.trueCenterFreq = trueCenterFreq;

        subSpectraAveraged += procNumber;
        totalProcessed += 1;


        // Acquire a new lock_guard and push the processed data to the shared queue
        {
            std::lock_guard<std::mutex> lock(sharedData.mutex);
            sharedData.rawDataQueue.push(rawSpectrum);
        }
        sharedData.rawDataReadyCondition.notify_one();
        
        lock.lock();  // Reacquire lock before checking the data queue
        stopTimer(TIMER_AVERAGE);

        // Check if the acquisition and processing is complete
        {
            std::lock_guard<std::mutex> lock(syncFlags.mutex);
            if (syncFlags.magnitudeComplete && sharedData.magDataQueue.empty()) {
                setMetric(ACQUIRED_SPECTRA, subSpectraAveraged);
                setMetric(SPECTRUM_AVERAGE_SIZE, subSpectraAveragingNumber);

                std::cout << "Averaging thread exiting. Averaged " << std::to_string(subSpectraAveraged) << " sub-spectra into " 
                                                                   << std::to_string(totalProcessed) << " spectra." << std::endl;

                syncFlags.averagingComplete = true;
                sharedData.rawDataReadyCondition.notify_one();
                break;  // Exit the processing thread
            }

            if (syncFlags.errorFlag) {
                std::cout << "Averaging thread gracefully exiting due to error." << std::endl;

                sharedData.rawDataReadyCondition.notify_one();
                break;
            }
        }
    }
    }
                
    catch (const std::exception& e) {
        std::lock_guard<std::mutex> lock(syncFlags.mutex);
        syncFlags.errorFlag = true;
        syncFlags.errorMessage = "AveragingThread: " + std::string(e.what());
        std::cout << syncFlags.errorMessage << '\n';
    }
}



/**
 * @brief Placeholder function to be run in a separate thread in parallel with ATS::AcquireDataMultithreadedContinuous. 
 *        Currently just pops data from the processed data queue and frees the memory.
 * 
 * @param sharedData - Struct containing data shared between threads
 * @param syncFlags - Struct containing synchronization flags shared between threads
 */
void processingThread(SharedDataProcessing& sharedData, SavedData& savedData, SynchronizationFlags& syncFlags, DataProcessor& dataProcessor, BayesFactors& bayesFactors) {
    try{
    int buffersProcessed = 0;
    while (true) {
        // Check if processed data queue is empty
        std::unique_lock<std::mutex> lock(sharedData.mutex);
        sharedData.rawDataReadyCondition.wait(lock, [&sharedData]() {
            return !sharedData.rawDataQueue.empty();
        });


        // Process data if the data queue is not empty
        startTimer(TIMER_PROCESS);
        while (!sharedData.rawDataQueue.empty()) {
            // Get the pointer to the data from the queue
            Spectrum rawSpectrum = sharedData.rawDataQueue.front();
            sharedData.rawDataQueue.pop();
            lock.unlock();

            Spectrum processedSpectrum, foo;
            std::tie(processedSpectrum, foo) = dataProcessor.rawToProcessed(rawSpectrum);

            trimSpectrum(processedSpectrum, 0.1);
            dataProcessor.trimSNRtoMatch(processedSpectrum);

            Spectrum rescaledSpectrum = dataProcessor.processedToRescaled(processedSpectrum);

            CombinedSpectrum combinedSpectrum;
            dataProcessor.addRescaledToCombined(rescaledSpectrum, combinedSpectrum);

            CombinedSpectrum rebinnedSpectrum = dataProcessor.rebinCombinedSpectrum(combinedSpectrum, 10, 1);

            // bayesFactors.updateExclusionLine(rebinnedSpectrum);

            buffersProcessed++;

            {
                std::lock_guard<std::mutex> lock(savedData.mutex);
                if (savedData.rawSpectra.size() < 10){
                    savedData.rawSpectra.push_back(rawSpectrum);
                }
                // savedData.processedSpectra.push_back(processedSpectrum);
                // savedData.rescaledSpectra.push_back(rescaledSpectrum);
                dataProcessor.addRescaledToCombined(rescaledSpectrum, savedData.combinedSpectrum);
            }

            // Acquire a new lock_guard and push the processed data to the shared queue
            {
                std::lock_guard<std::mutex> lock(sharedData.mutex);
                sharedData.rebinnedDataQueue.push(rebinnedSpectrum);
                sharedData.rescaledDataReadyCondition.notify_one();
            }

            lock.lock();  // Lock again before checking the data queue
        }
        stopTimer(TIMER_PROCESS);
        

        // Check if the acquisition is complete
        {
            std::lock_guard<std::mutex> lock(syncFlags.mutex);
            if (syncFlags.averagingComplete && sharedData.rawDataQueue.empty()) {
                std::cout << "Processing thread exiting. Processed " << std::to_string(buffersProcessed) << " spectra." << std::endl;

                syncFlags.processingComplete = true;
                sharedData.rescaledDataReadyCondition.notify_one();

                break;  // Exit the processing thread
            }

            if (syncFlags.errorFlag) {
                std::cout << "Processing thread gracefully exiting due to error." << std::endl;

                sharedData.rescaledDataReadyCondition.notify_one();
                break;
            }
        }
    }

    }
    catch(const std::exception& e)
    {
        std::lock_guard<std::mutex> lock(syncFlags.mutex);
        syncFlags.errorFlag = true;
        syncFlags.errorMessage = "ProcessingThread: " + std::string(e.what());
        std::cout << syncFlags.errorMessage << '\n';
    }
}





/**
 * @brief Placeholder function to be run in a separate thread in parallel with ATS::AcquireDataMultithreadedContinuous. 
 *        Currently just pops data from the processed data queue and frees the memory.
 * 
 * @param sharedData - Struct containing data shared between threads
 * @param syncFlags - Struct containing synchronization flags shared between threads
 */
void decisionMakingThread(SharedDataProcessing& sharedData, SharedDataSaving& savedData, SynchronizationFlags& syncFlags, BayesFactors& bayesFactors, DecisionAgent& decisionAgent) {
    try{
    setMetric(SPECTRA_AT_DECISION, -1);
    
    int buffersDecided = 0;
    bool decisionThrown = false;
    while (true) {
        // Check if processed data queue is empty
        std::unique_lock<std::mutex> lock(sharedData.mutex);
        sharedData.rescaledDataReadyCondition.wait(lock, [&sharedData]() {
            return !sharedData.rebinnedDataQueue.empty();
        });


        // Process data if the data queue is not empty
        startTimer(TIMER_DECISION);
        while (!sharedData.rebinnedDataQueue.empty()) {
            // Get the pointer to the data from the queue
            CombinedSpectrum rebinnedSpectrum = sharedData.rebinnedDataQueue.front();
            sharedData.rebinnedDataQueue.pop();
            lock.unlock();

            if (decisionAgent.trimmedSNR.powers.empty()) {
                decisionAgent.resizeSNRtoMatch(rebinnedSpectrum);
                decisionAgent.setTargets();
                decisionAgent.setPoints();
            }
            

            bayesFactors.updateExclusionLine(rebinnedSpectrum);

            #if SAVE_PROGRESS
            // Create a copy of the bayesFactors object and add it to the exclusionLineQueue
            {
                std::lock_guard<std::mutex> savedDataLock(savedData.mutex);
                Spectrum exclusionCopy = bayesFactors.exclusionLine;
                savedData.exclusionLineQueue.push(exclusionCopy);
                savedData.exclusionLineReadyCondition.notify_one();
            }
            #endif

            if (!decisionThrown){
                std::vector<double> activeWindow(bayesFactors.exclusionLine.powers.end() - decisionAgent.trimmedSNR.powers.size(), bayesFactors.exclusionLine.powers.end());
                int decision = decisionAgent.getDecision(activeWindow, buffersDecided);
                // int decision = 0;

                buffersDecided++;

                if (decision) {
                    decisionThrown = true;

                    std::lock_guard<std::mutex> lock(syncFlags.mutex);
                    syncFlags.acquisitionComplete = true;
                    syncFlags.pauseDataCollection = true;

                    updateMetric(SPECTRA_AT_DECISION, buffersDecided);
                    break;
                }
            }


            lock.lock();  // Lock again before checking the data queue
        }
        stopTimer(TIMER_DECISION);

        // Check if the acquisition is complete
        {
            std::lock_guard<std::mutex> lock(syncFlags.mutex);
            if (syncFlags.processingComplete && sharedData.rebinnedDataQueue.empty()) {

                if (!decisionThrown) {
                    updateMetric(SPECTRA_AT_DECISION, buffersDecided);
                }

                std::cout<< "Decision making thread exiting. Decided on " << std::to_string(buffersDecided) << " spectra." << std::endl;

                syncFlags.acquisitionComplete = true;
                syncFlags.pauseDataCollection = true;
                syncFlags.decisionsComplete = true;
                
                break;  // Exit the processing thread
            }

            if (syncFlags.errorFlag) {
                std::cout << "Decision making thread gracefully exiting due to error." << std::endl;

                if (!decisionThrown) {
                    updateMetric(SPECTRA_AT_DECISION, buffersDecided);
                }
                break;
            }
        }
    }
    }
    catch(const std::exception& e)
    {
        std::lock_guard<std::mutex> lock(syncFlags.mutex);
        syncFlags.errorFlag = true;
        syncFlags.errorMessage = "DecisionThread: " + std::string(e.what());
        std::cout << syncFlags.errorMessage << '\n';
    }
}



void dataSavingThread(SharedDataSaving& savedData, SynchronizationFlags& syncFlags) {
    try{
    int buffersSaved = 0;

    while (true) {
        Spectrum exclusionLine;

        // Check if processed data queue is empty
        std::unique_lock<std::mutex> lock(savedData.mutex);
        savedData.exclusionLineReadyCondition.wait(lock, [&savedData]() {
            return !savedData.exclusionLineQueue.empty();
        });

        // Process data if the data queue is not empty
        startTimer(TIMER_SAVE);
        while (!savedData.exclusionLineQueue.empty()) {
            exclusionLine = savedData.exclusionLineQueue.front();
            savedData.exclusionLineQueue.pop();

            lock.unlock();
            std::string exclusionLineFilename = "../../../plotting/exclusionLineComparisons/scanProgress/exclusionLine_" + 
                                                std::to_string(buffersSaved) + "_" + getDateTimeString() + ".csv";
            saveSpectrum(exclusionLine, exclusionLineFilename);
            buffersSaved++;
            lock.lock();
        }
        stopTimer(TIMER_SAVE);


        // Check if the acquisition is complete
        {
            std::lock_guard<std::mutex> lock(syncFlags.mutex);
            if (savedData.exclusionLineQueue.empty() && syncFlags.decisionsComplete) {
                std::cout<< "Saving thread exiting. Saved " << std::to_string(buffersSaved) << " spectra." << std::endl;
                break;  // Exit the processing thread
            }
        }
    }
    }
    catch(const std::exception& e)
    {
        std::cout << "Data saving thread exiting due to exception." << std::endl;
        std::cout << e.what() << '\n';
    }
}