#include "ATS.hpp"

#include <iostream>
#include <stdexcept>

#include <string>
#include <vector>

#include <cmath>
#include <algorithm>


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



double ATS::setExternalSampleClock(double requestedSampleRate) {
    // Fix sample rate to be positive and less than the absolute maximum of 180 MHz
    requestedSampleRate = min(std::abs(requestedSampleRate), 180e6);

    // Establish the range of bare sample rates the board is capable of (150 MHz - 180 MHz with steps of 1 MHz)
    std::vector<int> bareSampleRates;
    for (int rate = (int)150e6; rate <= (int)180e6; rate += (int)1e6) {
        bareSampleRates.push_back(rate);
    }

    // Find the best decimation factor for each bare sample rate
    std::vector<int> decimationFactors;
    std::vector<double> effectiveSampleRates;
    for (int& rate : bareSampleRates) {
        int factor = (int) min(std::round((double)rate/requestedSampleRate),10000);

        decimationFactors.push_back(factor);
        effectiveSampleRates.push_back((double)rate/(double)factor);
    }


    // Find the closest bare sample rate & decimation factor pair to approximate the requested sample rate
    int bestIndex = 0;
    double minError = DBL_MAX;
    for (int i=0; i < effectiveSampleRates.size(); i++){
        double error = std::abs(effectiveSampleRates[i] - requestedSampleRate);

        if (error < minError){
            minError = error;
            bestIndex = i;

            if (minError == 0) {
                break;
            }
        }
    }

    // Send the sample rate to the card
    RETURN_CODE retCode = AlazarSetCaptureClock(
        systemHandle,			        // HANDLE -- board handle
        EXTERNAL_CLOCK_10MHz_REF,		// U32 -- clock source id
        bareSampleRates[bestIndex],		// U32 -- sample rate id
        CLOCK_EDGE_RISING,		        // U32 -- clock edge id
        decimationFactors[bestIndex]    // U32 -- clock decimation 
    );

	if (retCode != ApiSuccess)
	{
		throw std::runtime_error(std::string("Error: AlazarSetCaptureClock failed -- ") + AlazarErrorToText(retCode) + "\n");
	}

    return effectiveSampleRates[bestIndex];
}