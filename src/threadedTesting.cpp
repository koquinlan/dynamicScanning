#include "matplotlibcpp.h"
namespace plt = matplotlibcpp;

#include <iostream>
#include <string>
#include <vector>

#include <chrono>
#include <thread>
#include <stdexcept>
#include <future>

#include "PSG.hpp"
#include "AWG.hpp"
#include "ATS.hpp"
#include "tests.hpp"


int main() {
    int numShots = 8;

    printAvailableResources();

    ATS alazarCard(1, 1);
    alazarCard.setAcquisitionParameters((U32)30e6, (U32)7.5e6, 0);

    std::vector<std::future<std::pair<std::vector<double>, std::vector<double>>>> futures;


    // Main acquisition and processing logic
    DWORD startTickCount = GetTickCount();
    for (int i = 0; i < numShots; i++) {
        std::pair<std::vector<unsigned short>, std::vector<unsigned short>> rawData = alazarCard.AcquireData();

        // Start a new async thread for processing each acquisition
        futures.push_back(std::async(std::launch::async, processData, rawData, alazarCard.acquisitionParams));
    }

    // Wait for all asynchronous tasks to complete
    for (auto& future : futures) {
        std::pair<std::vector<double>, std::vector<double>> procData = future.get();
    }

    double fullTime_sec = (GetTickCount() - startTickCount) / 1000.;
    printf("\nMultithreaded run completed in %.3lf sec\n\n", fullTime_sec);



    // Normal acquisition and processing logic
    startTickCount = GetTickCount();
    for (int i = 0; i < numShots; i++) {
        std::pair<std::vector<unsigned short>, std::vector<unsigned short>> rawData = alazarCard.AcquireData();

        // Start a new async thread for processing each acquisition
        std::pair<std::vector<double>, std::vector<double>> procData = processData(rawData, alazarCard.acquisitionParams);
    }

    fullTime_sec = (GetTickCount() - startTickCount) / 1000.;
    printf("\nSingle threaded run completed in %.3lf sec\n\n", fullTime_sec);

    return 0;
}
