#include "matplotlibcpp.h"
namespace plt = matplotlibcpp;

#include <iostream>
#include <string>
#include <vector>

#include <chrono>
#include <thread>
#include <stdexcept>

#include "PSG.hpp"
#include "AWG.hpp"
#include "ATS.hpp"
#include "tests.hpp"

int main() {
    printAvailableResources();

    // psgTesting(21);
    // awgTesting(10);

    try{
        ATS alazarCard(1, 1);

        alazarCard.setAcquisitionParameters((U32)32e6, (U32)1e6, 0);

        std::pair<std::vector<unsigned short>, std::vector<unsigned short>> rawData = alazarCard.AcquireData();

        std::pair<std::vector<double>, std::vector<double>> procData = processData(rawData, alazarCard.acquisitionParams);
        std::vector<double> channelDataA = procData.first;
        std::vector<double> channelDataB = procData.second;

        plt::plot(channelDataA);
        plt::plot(channelDataB);
        plt::show();
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }

    return 0;
}
