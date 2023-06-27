#include "matplotlibcpp.h"
namespace plt = matplotlibcpp;

#include <iostream>
#include <fstream>
#include <filesystem>

#include <string>
#include <vector>
#include <thread>

#include <chrono>
#include <stdexcept>
#include <future>

#include "PSG.hpp"
#include "AWG.hpp"
#include "ATS.hpp"
#include "tests.hpp"


void ProcessingThread(fftw_plan plan, int N, SharedData& sharedData, SynchronizationFlags& syncFlags) {
    int numProcessed = 0;
    while (true) {
        // Check if data queue is empty
        std::unique_lock<std::mutex> lock(sharedData.mutex);
        sharedData.dataReadyCondition.wait(lock, [&sharedData]() {
            return !sharedData.dataQueue.empty();
        });


        // Process data if the data queue is not empty
        while (!sharedData.dataQueue.empty()) {
            // Get the pointer to the data from the queue
            fftw_complex* complexOutput = sharedData.dataQueue.front();
            sharedData.dataQueue.pop();
            lock.unlock();

            // Process the data
            fftw_complex* procData = processDataFFT(complexOutput, plan, N);
            numProcessed++;
            // std::cout << "Processed " << std::to_string(numProcessed) << " buffers." << std::endl;

            // Reaquire a lock and push the processed data to the shared queue
            {
                std::lock_guard<std::mutex> lock(sharedData.mutex);
                sharedData.dataSavingQueue.push(complexOutput);
                sharedData.processedDataQueue.push(procData);
            }
            sharedData.processedDataReadyCondition.notify_one();
            sharedData.saveReadyCondition.notify_one();

            lock.lock();  // Lock again before checking the data queue
        }

        // Check if the acquisition is complete
        {
            std::lock_guard<std::mutex> lock(syncFlags.mutex);
            if (syncFlags.acquisitionComplete && sharedData.dataQueue.empty()) {
                break;  // Exit the processing thread
            }
        }
    }
}


void DecisionMakingThread(SharedData& sharedData, SynchronizationFlags& syncFlags) {
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
            fftw_complex* processedOutput = sharedData.processedDataQueue.front();
            sharedData.processedDataQueue.pop();
            lock.unlock();

            // Free the memory allocated for the raw data
            fftw_free(processedOutput);
            buffersProcessed++;
            // std::cout << "Decided on " << std::to_string(buffersProcessed) << " buffers." << std::endl;

            if (buffersProcessed >= 50) {
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




int main() {
    // std::chrono::seconds dura(5);
    // std::this_thread::sleep_for(dura);

    double sampleRate = 32e6;
    double samplesPerAcquisition = 7.5e7;

    // Try to import an FFTW plan if available
    const char* wisdomFilePath = "fftw_wisdom.txt";
    if (fftw_import_wisdom_from_filename(wisdomFilePath) != 0) {
        std::cout << "Successfully imported FFTW wisdom from file." << std::endl;
    }
    else {
        std::cout << "Failed to import FFTW wisdom from file." << std::endl;
    }

    // Prepare for acquisition and create FFTW plan
    ATS alazarCard(1, 1);
    alazarCard.setAcquisitionParameters((U32)sampleRate, (U32)samplesPerAcquisition, 0);
    std::cout << "Acquisition parameters set. Collecting " << std::to_string(alazarCard.acquisitionParams.buffersPerAcquisition) << " buffers." << std::endl;


    int N = (int)alazarCard.acquisitionParams.samplesPerBuffer;
    std::cout << "Creating plan for N = " << std::to_string(N) << std::endl;
    fftw_complex* fftwInput = reinterpret_cast<fftw_complex*>(fftw_malloc(sizeof(fftw_complex) * N));
    fftw_complex* fftwOutput = reinterpret_cast<fftw_complex*>(fftw_malloc(sizeof(fftw_complex) * N));
    fftw_plan plan = fftw_plan_dft_1d(N, fftwInput, fftwOutput, FFTW_FORWARD, FFTW_MEASURE);
    std::cout << "Plan created!" << std::endl;


    // Create shared data structures
    SharedData sharedData;
    SynchronizationFlags syncFlags;

    sharedData.samplesPerBuffer = N;

    try{
        // Start the threads
        std::thread acquisitionThread(&ATS::AcquireDataMultithreadedContinuous, &alazarCard, std::ref(sharedData), std::ref(syncFlags));
        std::thread processingThread(ProcessingThread, plan, N, std::ref(sharedData), std::ref(syncFlags));
        std::thread decisionMakingThread(DecisionMakingThread, std::ref(sharedData), std::ref(syncFlags));
        
        #if SAVE_DATA
        std::thread savingThread(saveDataToBin, std::ref(sharedData), std::ref(syncFlags));
        #endif

        // Wait for the threads to finish
        acquisitionThread.join();
        processingThread.join();
        decisionMakingThread.join();

        #if SAVE_DATA
        savingThread.join();
        #endif
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }


    // // Main acquisition and processing logic
    // DWORD startTickCount = GetTickCount();

    // double fullTime_sec = (GetTickCount() - startTickCount) / 1000.;
    // double expectedScanTime = (samplesPerAcquisition/sampleRate)*numShots;
    // printf("\nMultithreaded run completed in %.3lf sec\n", fullTime_sec);
    // printf("Time actively scanning %.3lf sec\n", scanTime_sec);
    // printf("Expected scan time %.3lf sec\n", expectedScanTime);
    // printf("Total live time efficiency: %.3lf \n\n", expectedScanTime/scanTime_sec);


    // Cleanup
    fftw_export_wisdom_to_filename(wisdomFilePath);
    std::cout << "FFTW wisdom saved to file." << std::endl;

    fftw_destroy_plan(plan);
    fftw_free(fftwInput);
    fftw_free(fftwOutput);

    return 0;
}
