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

        // Check if the acquisition and processing is complete
        {
            std::lock_guard<std::mutex> lock(syncFlags.mutex);
            if (syncFlags.acquisitionComplete && sharedData.dataQueue.empty()) {
                std::cout << "FFT thread exiting. Processed " << std::to_string(numProcessed) << " spectra." << std::endl;

                syncFlags.FFTComplete = true;
                sharedData.FFTDataReadyCondition.notify_one();
                break;  // Exit the processing thread
            }
        }
    }
}


void magnitudeThread(int samplesPerSpectrum, SharedDataBasic& sharedData, SharedDataProcessing& sharedDataProc, SynchronizationFlags& syncFlags, DataProcessor& dataProcessor) {
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

            magData = dataProcessor.removeBadBins(magData);
            
            // Free the memory allocated for the fft data
            fftw_free(FFTData);
            numProcessed++;


            // Acquire a new lock_guard and push the processed data to the shared queue
            {
                std::lock_guard<std::mutex> lock(sharedData.mutex);
                sharedDataProc.magDataQueue.push(magData);
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
        }
    }
    
}



void averagingThread(SharedDataProcessing& sharedData, SynchronizationFlags& syncFlags, DataProcessor& dataProcessor, double trueCenterFreq) {
    int subSpectraAveraged = 0;
    int totalProcessed = 0;
    int subSpectraAveragingNumber = 32;
    while (true) {
        // Wait for signal from dataReadyCondition or immediately continue if the data queue is not empty (lock releases while waiting)
        std::unique_lock<std::mutex> lock(sharedData.mutex);
        sharedData.magDataReadyCondition.wait(lock, [&sharedData, &subSpectraAveragingNumber, &syncFlags]() {
            return (sharedData.magDataQueue.size() >= subSpectraAveragingNumber) || syncFlags.magnitudeComplete;
        });

        startTimer(TIMER_AVERAGE);

        int procNumber = min(subSpectraAveragingNumber, (int) sharedData.magDataQueue.size());
        std::vector<std::vector<double>> subSpectra(procNumber);

        for(int i = 0; i < procNumber; i++) {
            // Get the pointer to the data from the queue
            subSpectra[i] = sharedData.magDataQueue.front();
            sharedData.magDataQueue.pop();

            // Update the running average using DataProcessor
            dataProcessor.addRawSpectrumToRunningAverage(subSpectra[i]);
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
                std::cout << "Averaging thread exiting. Averaged " << std::to_string(subSpectraAveraged) << " sub-spectra into " 
                                                                   << std::to_string(totalProcessed) << " spectra." << std::endl;

                syncFlags.averagingComplete = true;
                sharedData.rawDataReadyCondition.notify_one();
                break;  // Exit the processing thread
            }
        }
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

            bayesFactors.updateExclusionLine(combinedSpectrum);

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
                sharedData.rescaledDataQueue.push(rescaledSpectrum);
            }
            sharedData.rescaledDataReadyCondition.notify_one();

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
        }
    }
}





/**
 * @brief Placeholder function to be run in a separate thread in parallel with ATS::AcquireDataMultithreadedContinuous. 
 *        Currently just pops data from the processed data queue and frees the memory.
 * 
 * @param sharedData - Struct containing data shared between threads
 * @param syncFlags - Struct containing synchronization flags shared between threads
 */
void decisionMakingThread(SharedDataProcessing& sharedData, SynchronizationFlags& syncFlags, BayesFactors& bayesFactors) {
    int buffersDecided = 0;
    while (true) {
        // Check if processed data queue is empty
        std::unique_lock<std::mutex> lock(sharedData.mutex);
        sharedData.rescaledDataReadyCondition.wait(lock, [&sharedData]() {
            return !sharedData.rescaledDataQueue.empty();
        });


        // Process data if the data queue is not empty
        startTimer(TIMER_DECISION);
        while (!sharedData.rescaledDataQueue.empty()) {
            // Get the pointer to the data from the queue
            Spectrum rawSpectrum = sharedData.rescaledDataQueue.front();
            sharedData.rescaledDataQueue.pop();
            lock.unlock();


            buffersDecided++;

            if (buffersDecided >= 100) {
                std::lock_guard<std::mutex> lock(syncFlags.mutex);
                syncFlags.acquisitionComplete = true;
                syncFlags.pauseDataCollection = true;
                break;
            }

            lock.lock();  // Lock again before checking the data queue
        }
        stopTimer(TIMER_DECISION);

        // Check if the acquisition is complete
        {
            std::lock_guard<std::mutex> lock(syncFlags.mutex);
            if (syncFlags.processingComplete && sharedData.rescaledDataQueue.empty()) {
                std::cout<< "Decision making thread exiting. Decided on " << std::to_string(buffersDecided) << " spectra." << std::endl;
                break;  // Exit the processing thread
            }
        }
    }
}



/**
 * @brief Function to be run in a separate thread in parallel with ATS::AcquireDataMultithreadedContinuous. Saves data from the data 
 *        saving queue to binary files. The data is saved in the output directory.
 * 
 * @param sharedData - Struct containing data shared between threads
 * @param syncFlags - Struct containing synchronization flags shared between threads
 */
void saveDataToBin(SharedDataBasic& sharedData, SynchronizationFlags& syncFlags) {
    // Create the output directory
    std::filesystem::create_directory("output");

    int numSaved = 0;
    while (true) {
        // Check if data queue is empty
        std::unique_lock<std::mutex> lock(sharedData.mutex);
        sharedData.saveReadyCondition.wait(lock, [&sharedData]() {
            return !sharedData.dataSavingQueue.empty();
        });


        // Process data if the data queue is not empty
        startTimer(TIMER_SAVE);
        while (!sharedData.dataSavingQueue.empty()) {
            // Get the pointer to the data from the queue
            fftw_complex* rawData = sharedData.dataSavingQueue.front();
            sharedData.dataSavingQueue.pop();
            lock.unlock();
        

            std::string filename = "output/Buffer" + std::to_string(numSaved+1) + ".bin";
            // Open the file in binary mode
            std::ofstream file(filename, std::ios::binary);
            if (!file) {
                std::cout << "Failed to open the file." << std::endl;
            }

            // Write the complex data to the file
            file.write(reinterpret_cast<const char*>(rawData), sizeof(fftw_complex) * sharedData.samplesPerBuffer);

            // Close the file
            file.close();
            numSaved++;

            // Free the memory allocated for the raw data
            fftw_free(rawData);

            lock.lock();  // Lock again before checking the data queue
        }
        stopTimer(TIMER_SAVE);

        // Check if the acquisition and saving is complete
        {
            std::lock_guard<std::mutex> lock(syncFlags.mutex);
            if (syncFlags.acquisitionComplete && sharedData.dataSavingQueue.empty()) {
                break;
            }
        }
    }
}