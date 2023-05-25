#include "PSG.hpp"

#include <iostream>
#include <string>
#include <stdexcept>

#include <visa.h>


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
    int num;
    
    // std::string command = on ? "OUTPUT 1" : "OUTPUT 0";
    // ViStatus status = viPrintf(vi, "%s", command.c_str());

    viPrintf(vi, "OUTP:STAT ON\n"); // Turn source RF state on
    viPrintf(vi, "OUTP?\n"); // Query the signal generator’s RF state
    viScanf(vi, "%1i", &num); // Read the response (integer value) 

    // if (status != VI_SUCCESS) {
    //     throw std::runtime_error("Failed to set PSG on/off state.");
    // }
}



void PSG::setFreq(double frequency) {
    // Implement the logic to set the frequency of the PSG
    // Example code using viPrintf to send a command
}