#ifndef INST_H
#define INST_H

#include <iostream>
#include <string>

#include <visa.h>

class Instrument {
protected:
    ViSession instrumentSession;
    ViSession defaultRM;

    ViStatus status;

public:
    Instrument() {
        status = viOpenDefaultRM(&defaultRM);
        if (status != VI_SUCCESS) {
            std::cout << "Failed to initialize VISA." << std::endl;
            throw std::runtime_error("Failed to initialize VISA.");
        }
    }

    virtual ~Instrument() {
        viClose(instrumentSession);
        viClose(defaultRM);
        status = VI_NULL;
    }

    void openConnection(int gpibAddress) {
        std::string deviceAddress = "GPIB0::" + std::to_string(gpibAddress) + "::INSTR";
        status = viOpen(defaultRM, (ViRsrc)deviceAddress.c_str(), VI_NULL, VI_NULL, &instrumentSession);
        if (status != VI_SUCCESS) {
            throw std::runtime_error("Failed to open a connection to the instrument.");
        }

        char idnBuffer[256];
        status = viQueryf(instrumentSession, "*IDN?", "%t", idnBuffer);
        if (status != VI_SUCCESS) {
            throw std::runtime_error("Failed to query device identification");
        } else {
            std::string idn(idnBuffer);
            std::cout << "Query ID: " << idn;
        }
    }

    void closeConnection() {
        viClose(instrumentSession);
        instrumentSession = VI_NULL;
    }

    void onOff(bool on) {
        std::string command = on ? "OUTPUT 1" : "OUTPUT 0";
        status = viPrintf(instrumentSession, "%s\n", command.c_str());

        if (status != VI_SUCCESS) {
            throw std::runtime_error("Failed to set instrument on/off state.");
        }
    }
};

#endif // INST_H