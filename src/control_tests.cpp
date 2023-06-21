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

        alazarCard.setAcquisitionParameters((U32)30e6, (U32)7.5e6, 0);

        std::pair<std::vector<double>, std::vector<double>> fullData = processData(alazarCard.AcquireData(), alazarCard.acquisitionParams);
        std::vector<double> channelDataA = fullData.first;
        std::vector<double> channelDataB = fullData.second;

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
