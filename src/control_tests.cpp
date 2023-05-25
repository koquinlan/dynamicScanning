#include <iostream>
#include <string>
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


    PSG psg3(21);

    psg3.onOff(true);


    viClose(defaultRM);
    return 0;
}
