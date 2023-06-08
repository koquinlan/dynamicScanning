#ifndef ATS_H
#define ATS_H

#include <string>

#include "AlazarError.h"
#include "AlazarApi.h"
#include "AlazarCmd.h"

class ATS {
public:
    ATS(int systemId = 1, int boardId = 1);
    ~ATS();

    double setExternalSampleClock(double requestedSampleRate);
    void setInputParameters(char channel, std::string coupling, double inputRange=0.8, double inputImpedance=50);

private:
    HANDLE boardHandle;
    RETURN_CODE retCode;
};


#endif // ATS_H
