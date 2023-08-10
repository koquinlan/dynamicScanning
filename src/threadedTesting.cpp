/**
 * @file threadedTesting.cpp
 * @author Kyle Quinlan (kyle.quinlan@colorado.edu)
 * @brief For rapid prototyping and testing of multithreaded control and acquisition code. See src/controlTests.cpp for single threaded development.
 * @version 0.1
 * @date 2023-06-30
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#include "decs.hpp"

int main() {
    ScanRunner scanRunner;

    scanRunner.acquireData();
    scanRunner.saveData();

    return 0;
}
