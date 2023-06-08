#include "ATS.hpp"

#include <iostream>
#include <string>
#include <stdexcept>

void ATS::InitializeBoard(int systemId, int boardId) {
    // Initialize the first board

    // Get a handle to the board
    systemHandle = AlazarGetBoardBySystemID(systemId, boardId);
    if (systemHandle == nullptr) {
        throw std::runtime_error("Unable to open board system ID " + std::to_string(systemId) + " board ID " + std::to_string(boardId) + "\n");
    }
}