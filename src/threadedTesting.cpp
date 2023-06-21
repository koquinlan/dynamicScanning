#include "matplotlibcpp.h"
namespace plt = matplotlibcpp;

#include <iostream>
#include <string>
#include <vector>
#include <mutex>

#include <chrono>
#include <thread>
#include <future>
#include <stdexcept>

#include "PSG.hpp"
#include "AWG.hpp"
#include "ATS.hpp"
#include "tests.hpp"

struct ThreadData {
    std::vector<double> channelDataA;
    std::vector<double> channelDataB;
    std::mutex mutex;
};

void processDataThread(const std::string& filename, const AcquisitionParameters& acquisitionParams, ThreadData& threadData) {
    DWORD startTickCount = GetTickCount();

    std::pair<std::vector<double>, std::vector<double>> fullData = processData(filename, acquisitionParams);
    std::vector<double> channelDataA = fullData.first;
    std::vector<double> channelDataB = fullData.second;

    // Lock the mutex before accessing the shared data structure
    std::lock_guard<std::mutex> lock(threadData.mutex);
    // Store the data in the shared data structure
    threadData.channelDataA = channelDataA;
    threadData.channelDataB = channelDataB;

    // Store processing time for this shot
    double processTime_sec = (GetTickCount() - startTickCount) / 1000.;
}

int main() {
    printAvailableResources();

    ATS alazarCard(1, 1);
    alazarCard.setAcquisitionParameters((U32)30e6, (U32)7.5e6, 0);

    const int numAcquisitions = 10;
    std::vector<std::future<void>> futures;
    std::vector<ThreadData> threadDataList(numAcquisitions); // Shared data structure for each thread

    // Start the data acquisition
    for (int i = 0; i < numAcquisitions; i++) {
        std::string filename = "data_" + std::to_string(i) + ".bin";
        FILE* fpData = fopen(filename.c_str(), "wb");
        alazarCard.AcquireData(fpData);
        if (fpData != nullptr)
            fclose(fpData);

        // Start processing the data in a separate thread
        futures.push_back(std::async(std::launch::async, processDataThread, std::cref(filename), std::cref(alazarCard.acquisitionParams), std::ref(threadDataList[i])));
    }

    // Wait for all processing threads to finish
    for (auto& future : futures) {
        future.wait();
    }

    // Access the processed data from the main thread
    for (int i = 0; i < numAcquisitions; i++) {
        // Lock the mutex before accessing the shared data structure
        std::lock_guard<std::mutex> lock(threadDataList[i].mutex);
        std::vector<double> channelDataA = threadDataList[i].channelDataA;
        std::vector<double> channelDataB = threadDataList[i].channelDataB;

        std::cout << std::endl << channelDataA.size() << std::endl;
    }

    return 0;
}
