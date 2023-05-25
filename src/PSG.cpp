#include "PSG.hpp"

#include <iostream>
#include <string>
#include <stdexcept>



PSG::PSG(int gpibAddress) {
    ViSession defaultRM;
    ViStatus status;

    status = viOpenDefaultRM(&defaultRM);
    if (status != VI_SUCCESS) {
        throw std::runtime_error("Failed to initialize VISA.");
    }

    std::string deviceAddress = "GPIB0::" + std::to_string(gpibAddress) + "::INSTR";
    status = viOpen(defaultRM, (ViRsrc)deviceAddress.c_str(), VI_NULL, VI_NULL, &vi);
    if (status != VI_SUCCESS) {
        viClose(defaultRM);
        throw std::runtime_error("Failed to open a connection to the PSG.");
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