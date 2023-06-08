#include "PSG.hpp"
#include "AWG.hpp"
#include "ATS.hpp"

#include <stdexcept>
#include <chrono>
#include <thread>

#include <iostream>
#include <string>
#include <vector>


void printAvailableResources() {
    ViFindList findList;
    ViUInt32 count;
    ViChar resources[VI_FIND_BUFLEN];
    ViStatus status;
    ViSession defaultRM;

    status = viOpenDefaultRM(&defaultRM);
    if (status != VI_SUCCESS) {
        std::cout << "Failed to initialize VISA." << std::endl;
        return;
    }

    status = viFindRsrc(defaultRM, "?*INSTR", &findList, &count, resources);
    if (status != VI_SUCCESS) {
        std::cout << "Failed to find resources." << std::endl;
        return;
    }

    std::cout << "Available Resources:" << std::endl;
    std::cout << resources << std::endl;
    while (status == VI_SUCCESS) {
        status = viFindNext(findList, resources);
        if (status == VI_SUCCESS) {
            std::cout << resources << std::endl;
        }
    }

    viClose(findList);
    viClose(defaultRM);
}

void psgTesting(int gpibAdress){
    try {
        /** PSG TESTING **/
        PSG psg3(gpibAdress);


        /** ON/OFF TOGGLE TEST **/
        psg3.onOff(true);
        std::this_thread::sleep_for(std::chrono::seconds(2));  // Delay for 1 second
        psg3.onOff(false);

        std::this_thread::sleep_for(std::chrono::seconds(1));


        /** FREQUENCY SETTING TEST **/
        psg3.setFreq(3.141592653);
        std::this_thread::sleep_for(std::chrono::seconds(2));
        psg3.setFreq(40);

        std::this_thread::sleep_for(std::chrono::seconds(1));


        /** POWER SETTING TEST **/
        psg3.setPow(-21.212121);
        std::this_thread::sleep_for(std::chrono::seconds(2));
        psg3.setPow(-105);

        std::this_thread::sleep_for(std::chrono::seconds(1));


        /** MODULATION TEST **/
        psg3.modOnOff(true);
        psg3.freqModOnOff(true);
        psg3.setFreqModDev(30);
        psg3.setFreqModSrc(true, 1);
        std::this_thread::sleep_for(std::chrono::seconds(2));
        psg3.modOnOff(false);
        psg3.freqModOnOff(false);
        psg3.setFreqModDev(1);
        psg3.setFreqModSrc(false, 1);
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
}

void awgTesting(int gpibAdress){
    try {
        /** AWG TESTING **/
        AWG awg(gpibAdress);

        awg.reset();

        /** WAVEFORM SENDING TEST **/
        std::vector<double> waveform;
        int count = 50000;
        for (int i = 0; i <= count; i++) {
            waveform.push_back(static_cast<double>(i));
        }

        awg.clearMemory();
        awg.sendWaveform(waveform, "TestArb", 1);

        std::this_thread::sleep_for(std::chrono::seconds(1));


        /** ON/OFF TOGGLE TEST **/
        awg.onOff(true);
        std::this_thread::sleep_for(std::chrono::seconds(2));  // Delay for 1 second
        awg.onOff(false);

        std::this_thread::sleep_for(std::chrono::seconds(1));


        // /** WAVEFORM SETTNG TEST **/
        // // std::string arbName = "INT:\\builtin\\cardiac.arb";
        // std::string arbName = "INT:\\builtin\\cardiac.arb";
        // awg.sendCustomCommand("SOURce1:FUNCtion ARB");
        // awg.sendCustomCommand("MMEM:LOAD:DATA1 '" + arbName + "'");
        // awg.sendCustomCommand("SOURce1:FUNCtion:ARB 'INT:\\" + arbName + ".arb'");
    }
    catch(const std::exception& e) {
        std::cerr << e.what() << '\n';
    }
}