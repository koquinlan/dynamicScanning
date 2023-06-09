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

        alazarCard.setExternalSampleClock(10e6);

        alazarCard.setInputParameters('a', "dc", 0.8);
        alazarCard.setBandwidthLimit('a', 1);

        alazarCard.setInputParameters('b', "dc", 0.8);
        alazarCard.setBandwidthLimit('b', 1);

        alazarCard.AcquireData();
        std::vector<double> fullData = alazarCard.processData();
        std::vector<double> recordData(fullData.begin(), fullData.begin() + fullData.size()/2);

        plt::plot(recordData);
        plt::show();
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }

    return 0;
}
