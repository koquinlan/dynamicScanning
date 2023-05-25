#include <iostream>
#include <visa.h>

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
    ViSession defaultRM;
    ViStatus status;

    status = viOpenDefaultRM(&defaultRM);
    if (status != VI_SUCCESS) {
        std::cout << "Failed to initialize VISA." << std::endl;
        return 1;
    }

    printAvailableResources(defaultRM);



    ViSession vi;

    // Open a connection to the GPIB device
    ViRsrc deviceAddress = "GPIB0::21::INSTR";  // Replace with your device's address
    status = viOpen(defaultRM, deviceAddress, VI_NULL, VI_NULL, &vi);
    if (status != VI_SUCCESS) {
        std::cout << "Failed to open a connection to the device." << std::endl;
        viClose(defaultRM);
        return 1;
    }

    // Send a command to the device
    const char* command = "*IDN?";  // Replace with your desired command
    status = viPrintf(vi, "%s\n", command);
    if (status != VI_SUCCESS) {
        std::cout << "Failed to send command to the device." << std::endl;
        viClose(vi);
        viClose(defaultRM);
        return 1;
    }

    // Read the response from the device
    char response[256];
    status = viScanf(vi, "%t", response);
    if (status != VI_SUCCESS) {
        std::cout << "Failed to read the response from the device." << std::endl;
        viClose(vi);
        viClose(defaultRM);
        return 1;
    }

    // Print the response
    std::cout << "Response: " << response << std::endl;

    // Close the connection and clean up
    viClose(vi);
    viClose(defaultRM);

    return 0;
}
