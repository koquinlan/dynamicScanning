#ifndef ATS_H
#define ATS_H

#include <string>
#include <vector>

#include "AlazarError.h"
#include "AlazarApi.h"
#include "AlazarCmd.h"
#include "IoBuffer.h"

#define BUFFER_COUNT 8

struct AcquisitionParameters {
    U32 sampleRate;
    U32 samplesPerAcquisition;
    U32 buffersPerAcquisition;
    U32 recordsPerAcquisition;

    double inputRange;

    U32 samplesPerBuffer;
    U32 bytesPerSample;
    U32 bytesPerBuffer;

    std::string filename;
};


class ATS {
public:
    ATS(int systemId = 1, int boardId = 1);
    ~ATS();

    double setExternalSampleClock(double requestedSampleRate);
    void setAcquisitionParameters(U32 sampleRate, U32 samplesPerAcquisition, U32 buffersPerAcquisition=1, double inputRange=0.8);
    void setInputParameters(char channel, std::string coupling, double inputRange, double inputImpedance=50);
    void setBandwidthLimit(char channel, bool limit);

    void AcquireData(FILE *fpData);
    std::pair<std::vector<double>, std::vector<double>> processData(std::string filename);

    U32 suggestBufferNumber(U32 sampleRate, U32 samplesPerAcquisition);

private:
    HANDLE boardHandle;
    RETURN_CODE retCode;

    IO_BUFFER *IoBufferArray[BUFFER_COUNT] = { NULL };
    AcquisitionParameters acquisitionParams;

    int getChannelID(char channel);
};


#endif // ATS_H
