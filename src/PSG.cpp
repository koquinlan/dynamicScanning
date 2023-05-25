#include "PSG.hpp"
#include <iostream>

PSG::PSG(const char* deviceAddress) {
    ViSession defaultRM;
    ViStatus status;

    status = viOpenDefaultRM(&defaultRM);
    if (status != VI_SUCCESS) {
        std::cout << "Failed to initialize VISA." << std::endl;
        // Handle the error or throw an exception
        return;
    }

    status = viOpen(defaultRM, deviceAddress, VI_NULL, VI_NULL, &vi);
    if (status != VI_SUCCESS) {
        std::cout << "Failed to open a connection to the PSG." << std::endl;
        viClose(defaultRM);
        // Handle the error or throw an exception
        return;
    }

    viClose(defaultRM);
}



PSG::~PSG() {
    viClose(vi);
}



void PSG::onOff(bool on) {
    // Implement the logic to turn on/off the PSG
    // Example code using viPrintf to send a command
}



void PSG::setFreq(double frequency) {
    // Implement the logic to set the frequency of the PSG
    // Example code using viPrintf to send a command
}