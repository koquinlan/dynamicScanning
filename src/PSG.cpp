#include "PSG.hpp"

#include <iostream>
#include <string>
#include <stdexcept>

#include <visa.h>


PSG::PSG(int gpibAddress) {
    ViStatus status;

    status = viOpenDefaultRM(&defaultRM);
    if (status != VI_SUCCESS) {
        throw std::runtime_error("Failed to initialize VISA.");
    }

    std::string deviceAddress = "GPIB0::" + std::to_string(gpibAddress) + "::INSTR";
    status = viOpen(defaultRM, (ViRsrc)deviceAddress.c_str(), VI_NULL, VI_NULL, &vi);
    if (status != VI_SUCCESS) {
        throw std::runtime_error("Failed to open a connection to the PSG.");
    }

    char idnBuffer[256];
    status = viQueryf(vi, "*IDN?", "%t", idnBuffer);
    if (status != VI_SUCCESS) {
        throw std::runtime_error("Failed to query device identification");
    } else {
        std::string idn(idnBuffer);
        std::cout << "Query ID: " << idn;
    }
}



PSG::~PSG() {
    viClose(vi);
    viClose(defaultRM);
}



void PSG::onOff(bool on) {
    std::string command = on ? "OUTPUT 1" : "OUTPUT 0";
    ViStatus status = viPrintf(vi, "%s\n", command.c_str());

    if (status != VI_SUCCESS) {
        throw std::runtime_error("Failed to set PSG on/off state.");
    }
}



void PSG::setFreq(double frequency) {
    std::string command = "FREQ " + std::to_string(frequency) + "GHz";
    ViStatus status = viPrintf(vi, "%s\n", command.c_str());
    
    if (status != VI_SUCCESS) {
        throw std::runtime_error("Failed to set PSG frequency.");
    }
}
