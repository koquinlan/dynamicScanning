#ifndef ATS_H
#define ATS_H

#include <string>
#include <vector>

#include "AlazarError.h"
#include "AlazarApi.h"
#include "AlazarCmd.h"
#include "IoBuffer.h"

#define BUFFER_COUNT 4

class ATS {
public:
    ATS(int systemId = 1, int boardId = 1);
    ~ATS();

    double setExternalSampleClock(double requestedSampleRate);
    void setInputParameters(char channel, std::string coupling, double inputRange=0.8, double inputImpedance=50);
    void setBandwidthLimit(char channel, bool limit);

    void AcquireData();
    std::vector<double> processData();

private:
    HANDLE boardHandle;
    RETURN_CODE retCode;

    IO_BUFFER *IoBufferArray[BUFFER_COUNT] = { NULL };

    int getChannelID(char channel);
};


#endif // ATS_H
