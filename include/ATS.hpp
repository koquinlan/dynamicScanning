#ifndef ATS_H
#define ATS_H

#include <string>
#include <vector>

#include "AlazarError.h"
#include "AlazarApi.h"
#include "AlazarCmd.h"
#include "IoBuffer.h"

#define BUFFER_COUNT 4

struct AcquisitionParameters {
    U32 sampleRate;
    U32 samplesPerAcquisition;
    U32 buffersPerAcquisition;
    double inputRange;
    U32 samplesPerBuffer;
    U32 bytesPerSample;

    std::string filename;
};


class ATS {
public:
    ATS(int systemId = 1, int boardId = 1);
    ~ATS();

    double setExternalSampleClock(double requestedSampleRate);
    void setInputParameters(char channel, std::string coupling, double inputRange, double inputImpedance=50);
    void setBandwidthLimit(char channel, bool limit);

    void AcquireData(U32 sampleRate, U32 samplesPerAcquisition, U32 buffersPerAcquisition=1, double inputRange=0.8);
    std::pair<std::vector<double>, std::vector<double>> processData();

private:
    HANDLE boardHandle;
    RETURN_CODE retCode;

    IO_BUFFER *IoBufferArray[BUFFER_COUNT] = { NULL };
    AcquisitionParameters acquisitionParams;

    int getChannelID(char channel);
};


#endif // ATS_H
