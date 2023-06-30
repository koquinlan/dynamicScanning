#include "decs.hpp"


PSG::PSG(int gpibAddress) {
    openConnection(gpibAddress);
}



PSG::~PSG() {
    closeConnection();
}



void PSG::setFreq(double frequency) {
    std::string command = "FREQ " + std::to_string(frequency) + "GHz";
    status = viPrintf(instrumentSession, "%s\n", command.c_str());

    if (status != VI_SUCCESS) {
        throw std::runtime_error("Failed to set PSG frequency.");
    }
}



void PSG::setPow(double pow) {
    std::string command = "POW " + std::to_string(pow) + "dBm";
    status = viPrintf(instrumentSession, "%s\n", command.c_str());

    if (status != VI_SUCCESS) {
        throw std::runtime_error("Failed to set PSG power.");
    }
}



void PSG::modOnOff(bool on) {
    std::string command = on ? "OUTPUT:MOD 1" : "OUTPUT:MOD 0";
    status = viPrintf(instrumentSession, "%s\n", command.c_str());

    if (status != VI_SUCCESS) {
        throw std::runtime_error("Failed to set PSG overall modulation on/off state.");
    }
}



void PSG::freqModOnOff(bool on) {
    std::string command = on ? "FM:STATE 1" : "FM:STATE 0";
    status = viPrintf(instrumentSession, "%s\n", command.c_str());

    if (status != VI_SUCCESS) {
        throw std::runtime_error("Failed to set PSG FM on/off state.");
    }
}



void PSG::setFreqModDev(double dev) {
    std::string command = "FM:DEV " + std::to_string(dev) + "kHz";

    status = viPrintf(instrumentSession, "%s\n", command.c_str());

    if (status != VI_SUCCESS) {
        throw std::runtime_error("Failed to set FM modulation path deviation");
    }
}




void PSG::setFreqModSrc(bool ext, int src) {
    std::string command = "FM:SOURCE ";
    command += ext ? "EXT" : "INT";
    command += std::to_string(src);

    status = viPrintf(instrumentSession, "%s\n", command.c_str());

    if (status != VI_SUCCESS) {
        throw std::runtime_error("Failed to set FM modulation path deviation");
    }
}