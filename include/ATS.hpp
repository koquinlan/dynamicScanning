#ifndef ATS_H
#define ATS_H

#include "AlazarError.h"
#include "AlazarApi.h"
#include "AlazarCmd.h"

class ATS {
public:
    ATS(int systemId = 1, int boardId = 1);
    ~ATS();

    double setExternalSampleClock(double requestedSampleRate);

private:
    HANDLE systemHandle;
};


#endif // ATS_H
