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
 * @param N - Number of samples per buffer shared in the data queue
 * @param sharedData - Struct containing data shared between threads
 * @param syncFlags - Struct containing synchronization flags shared between threads
 */
void FFTThread(fftw_plan plan, int N, SharedData& sharedData, SynchronizationFlags& syncFlags) {
    int numProcessed = 0;
    while (true) {
        // std::cout << "Waiting for data..." << std::endl;

        // Wait for signal from dataReadyCondition or immediately continue if the data queue is not empty (lock releases while waiting)
        std::unique_lock<std::mutex> lock(sharedData.mutex);
        sharedData.dataReadyCondition.wait(lock, [&sharedData]() {
            return !sharedData.dataQueue.empty();
        });


        // Process data until data queue is empty (lock is reaquired before checking the data queue)
        while (!sharedData.dataQueue.empty()) {

            // Get the pointer to the data from the queue
            fftw_complex* complexOutput = sharedData.dataQueue.front();
            sharedData.dataQueue.pop();
            lock.unlock();

            // Process the data (lock is released while processing)
            fftw_complex* FFTData = processDataFFT(complexOutput, plan, N);
            numProcessed++;


            // Acquire a new lock_guard and push the processed data to the shared queue
            {
                std::lock_guard<std::mutex> lock(sharedData.mutex);
                sharedData.dataSavingQueue.push(complexOutput);
                sharedData.FFTDataQueue.push(FFTData);
            }
            sharedData.FFTDataReadyCondition.notify_one();
            sharedData.saveReadyCondition.notify_one();
            
            lock.lock();  // Reacquire lock before checking the data queue
        }

        // Check if the acquisition and processing is complete
        {
            std::lock_guard<std::mutex> lock(syncFlags.mutex);
            if (syncFlags.acquisitionComplete && sharedData.dataQueue.empty()) {
                break;  // Exit the processing thread
            }
        }
    }
}


void magnitudeThread(int N, SharedData& sharedData, SynchronizationFlags& syncFlags) {
    int numProcessed = 0;
    while (true) {
        // std::cout << "Waiting for data..." << std::endl;

        // Wait for signal from dataReadyCondition or immediately continue if the data queue is not empty (lock releases while waiting)
        std::unique_lock<std::mutex> lock(sharedData.mutex);
        sharedData.FFTDataReadyCondition.wait(lock, [&sharedData]() {
            return !sharedData.FFTDataQueue.empty();
        });


        // Process data until data queue is empty (lock is reaquired before checking the data queue)
        while (!sharedData.FFTDataQueue.empty()) {

            // Get the pointer to the data from the queue
            fftw_complex* FFTData = sharedData.FFTDataQueue.front();
            sharedData.FFTDataQueue.pop();
            lock.unlock();

            std::vector<double> procData(N);
            for (int i = 0; i < N; i++) {
                procData[i] = std::abs(std::complex<double>(FFTData[i][0], FFTData[i][1]));
            }

            // Free the memory allocated for the fft data
            fftw_free(FFTData);
            numProcessed++;


            // Acquire a new lock_guard and push the processed data to the shared queue
            {
                std::lock_guard<std::mutex> lock(sharedData.mutex);
                sharedData.processedDataQueue.push(procData);
            }
            sharedData.processedDataReadyCondition.notify_one();
            
            lock.lock();  // Reacquire lock before checking the data queue
        }

        // Check if the acquisition and processing is complete
        {
            std::lock_guard<std::mutex> lock(syncFlags.mutex);
            if (syncFlags.acquisitionComplete && sharedData.FFTDataQueue.empty()) {
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
void decisionMakingThread(SharedData& sharedData, SynchronizationFlags& syncFlags) {
    int buffersProcessed = 0;
    while (true) {
        // Check if processed data queue is empty
        std::unique_lock<std::mutex> lock(sharedData.mutex);
        sharedData.processedDataReadyCondition.wait(lock, [&sharedData]() {
            return !sharedData.processedDataQueue.empty();
        });


        // Process data if the data queue is not empty
        while (!sharedData.processedDataQueue.empty()) {
            // Get the pointer to the data from the queue
            std::vector<double> processedOutput = sharedData.processedDataQueue.front();
            sharedData.processedDataQueue.pop();
            lock.unlock();


            buffersProcessed++;
            // std::cout << "Decided on " << std::to_string(buffersProcessed) << " buffers." << std::endl;

            if (buffersProcessed >= 100) {
                std::lock_guard<std::mutex> lock(syncFlags.mutex);
                syncFlags.acquisitionComplete = true;
                syncFlags.pauseDataCollection = true;
                break;
            }

            lock.lock();  // Lock again before checking the data queue
        }

        // Check if the acquisition is complete
        {
            std::lock_guard<std::mutex> lock(syncFlags.mutex);
            if (syncFlags.acquisitionComplete) {
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
void saveDataToBin(SharedData& sharedData, SynchronizationFlags& syncFlags) {
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

        // Check if the acquisition and saving is complete
        {
            std::lock_guard<std::mutex> lock(syncFlags.mutex);
            if (syncFlags.acquisitionComplete && sharedData.dataSavingQueue.empty()) {
                break;
            }
        }
    }
}