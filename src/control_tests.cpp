#include <iostream>
#include <string>

#include <chrono>
#include <thread>

#include <visa.h>

#include "PSG.hpp"
#include "AWG.hpp"
#include "tests.hpp"

int main() {
    printAvailableResources();

    psgTesting(21);
    awgTesting(10);

    return 0;
}
