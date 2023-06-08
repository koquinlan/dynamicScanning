#include "ATS.hpp"

#include <iostream>
#include <string>
#include <stdexcept>

ATS::ATS(int systemId, int boardId) {
    systemHandle = AlazarGetBoardBySystemID(systemId, boardId);
    if (systemHandle == NULL) {
        throw std::runtime_error("Unable to open board system ID " + std::to_string(systemId) + " board ID " + std::to_string(boardId) + "\n");
    }
}

ATS::~ATS() { 
    if (systemHandle != NULL) {
        AlazarClose(systemHandle);
        systemHandle = NULL;
    }
}