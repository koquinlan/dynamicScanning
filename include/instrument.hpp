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

    void reset(){
        std::string command = "*RST";
        status = viPrintf(instrumentSession, "%s\n", command.c_str());

        if (status != VI_SUCCESS) {
            throw std::runtime_error("Failed to reset instrument");
        }

        queryError();
    }

    virtual void onOff(bool on) {
        std::string command = on ? "OUTPUT 1" : "OUTPUT 0";
        status = viPrintf(instrumentSession, "%s\n", command.c_str());

        if (status != VI_SUCCESS) {
            throw std::runtime_error("Failed to set instrument on/off state.");
        }
    }

    std::string queryError() {
        const int bufferSize = 256;
        char errorBuffer[bufferSize] = {0};
        ViUInt32 retCount = 0;

        status = viQueryf(instrumentSession, "SYST:ERR?", "%t", errorBuffer, bufferSize, &retCount);
        if (status != VI_SUCCESS) {
            throw std::runtime_error("Failed to query instrument error.");
        }

        std::string errorString(errorBuffer);
        if (errorString.substr(0, 2) != "+0") {
            throw std::runtime_error("Instrument reported an error: " + errorString);
        }

        return errorString;
    }
};

#endif // INST_H