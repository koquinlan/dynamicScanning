#ifndef ATS_H
#define ATS_H

#include "AlazarError.h"
#include "AlazarApi.h"
#include "AlazarCmd.h"

class ATS {
public:
    ATS() : systemHandle(nullptr) {}
    ~ATS() { 
        if (systemHandle != nullptr) {
            AlazarClose(systemHandle);
            systemHandle = nullptr;
        }
    }

    void InitializeBoard(int systemId = 1, int boardId = 1);

    // Other member functions...

private:
    HANDLE systemHandle;
};


#endif // ATS_H
