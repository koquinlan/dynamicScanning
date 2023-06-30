#include "decs.hpp"

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