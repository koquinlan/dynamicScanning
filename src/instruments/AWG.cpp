/**
 * @file AWG.cpp
 * @author Kyle Quinlan (kyle.quinlan@colorado.edu)
 * @brief Method definitions and documentation for the AWG class. See include\AWG.hpp for the class definition.
 * @version 0.1
 * @date 2023-06-30
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include "decs.hpp"

AWG::AWG(int gpibAddress){
    openConnection(gpibAddress);
}

AWG::~AWG(){
    closeConnection();
}

void AWG::onOff(bool on) {
    onOff(on, 1); // If onOff called with no channel, set channel to 1 and call
}

void AWG::onOff(bool on, int channel) {
    std::string command = on ? "OUTPUT" + std::to_string(channel) + " ON" : "OUTPUT" + std::to_string(channel) + " OFF";
    status = viPrintf(instrumentSession, "%s\n", command.c_str());

    if (status != VI_SUCCESS) {
        throw std::runtime_error("Failed to set AWG on/off state.");
    }

    queryError();
}

void AWG::setSampleRate(double rate, int channel){
    std::string command = "SOURCE" + std::to_string(channel) + ":FUNCtion:ARB:SRATe " + std::to_string(rate);
    status = viPrintf(instrumentSession, "%s\n", command.c_str());

    if (status != VI_SUCCESS) {
        throw std::runtime_error("Failed to set AWG sample rate.");
    }

    queryError();
}

void AWG::setAmp(double amp, int channel){
    std::string command = "SOURCE" + std::to_string(channel) + ":VOLT " + std::to_string(amp);
    status = viPrintf(instrumentSession, "%s\n", command.c_str());

    if (status != VI_SUCCESS) {
        throw std::runtime_error("Failed to set AWG amplitude.");
    }

    queryError();
}

void AWG::setOffset(double offset, int channel){
    std::string command = "SOURCE" + std::to_string(channel) + ":VOLT:OFFSET " + std::to_string(offset);
    status = viPrintf(instrumentSession, "%s\n", command.c_str());

    if (status != VI_SUCCESS) {
        throw std::runtime_error("Failed to set AWG dc offset.");
    }

    queryError();
}




void AWG::clearMemory(int channel){
    std::string command = "SOURCE" + std::to_string(channel) + ":DATA:VOLatile:CLEar";
    status = viPrintf(instrumentSession, "%s\n", command.c_str());

    if (status != VI_SUCCESS) {
        throw std::runtime_error("Failed to set AWG dc offset.");
    }

    queryError();
}



void AWG::sendWaveform(std::vector<double> waveform, std::string name, int channel) {
    // Scale arb to send
    double mx = *std::max_element(waveform.begin(), waveform.end());
    for (double& value : waveform) {
        value /= mx;
    }


    // Convert the waveform data to a comma-separated string
    std::stringstream waveformStream;
    for (const auto& sample : waveform) {
        waveformStream << sample << ",";
    }
    std::string waveformString = waveformStream.str();
    waveformString.pop_back(); // Remove the trailing comma


    // Send the waveform data to the AWG
    std::string command = "DATA" + std::to_string(channel) + ":ARB " + name + ", " + waveformString;
    status = viPrintf(instrumentSession, "%s\n", command.c_str());
    if (status != VI_SUCCESS) {
        throw std::runtime_error("Failed to send waveform to AWG.");
    }

    status = viPrintf(instrumentSession, "*WAI\n");
    queryError();



    // Set the AWG to use the uploaded waveform
    command = "SOURce" + std::to_string(channel) + ":FUNCtion ARB";
    status = viPrintf(instrumentSession, "%s\n", command.c_str());

    // command = "MMEM:LOAD:DATA" + std::to_string(channel) + " 'INT:\\" + name + ".arb'";
    // status = viPrintf(instrumentSession, "%s\n", command.c_str());

    command = "SOURce" + std::to_string(channel) + ":FUNCtion:ARB '" + name +"'";
    status = viPrintf(instrumentSession, "%s\n", command.c_str());

    if (status != VI_SUCCESS) {
        throw std::runtime_error("Failed to set AWG waveform.");
    }
    queryError();
}