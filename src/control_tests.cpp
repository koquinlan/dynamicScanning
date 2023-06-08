#include <iostream>
#include <string>

#include <chrono>
#include <thread>
#include <stdexcept>

#include "PSG.hpp"
#include "AWG.hpp"
#include "ATS.hpp"
#include "tests.hpp"

int main() {
    printAvailableResources();

    psgTesting(21);
    awgTesting(10);

    try{
        ATS alazarCard(1, 1);
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }

    return 0;
}
