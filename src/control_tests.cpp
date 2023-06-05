#include <iostream>
#include <string>

#include <chrono>
#include <thread>

#include <visa.h>

#include "PSG.hpp"


void printAvailableResources(ViSession session) {
    ViFindList findList;
    ViUInt32 count;
    ViChar resources[VI_FIND_BUFLEN];
    ViStatus status;

    status = viFindRsrc(session, "?*INSTR", &findList, &count, resources);
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
}

int main() {
    int gpibAddress = 21;

    ViSession defaultRM;
    ViStatus status;

    status = viOpenDefaultRM(&defaultRM);
    if (status != VI_SUCCESS) {
        std::cout << "Failed to initialize VISA." << std::endl;
        return 1;
    }

    printAvailableResources(defaultRM);


    PSG psg3(gpibAddress);

    try
    {
        /** ON/OFF TOGGLE TEST **/
        psg3.onOff(true);
        std::this_thread::sleep_for(std::chrono::seconds(1));  // Delay for 1 second
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

    viClose(defaultRM);
    return 0;
}
