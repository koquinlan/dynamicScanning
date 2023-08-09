#include "decs.hpp"



/**
 * @brief Call initParams to initalize parameters and set the data to it's starting value
 * 
 * @param parametersFile Text file to pass into initParams, lines of the form 'name=value'
 */
void BayesFactors::init(CombinedSpectrum combinedSpectrum) {
    // Clear any existing data
    exclusionLine.powers.clear();
    exclusionLine.freqAxis.clear();
    exclusionLine.trueCenterFreq = 0;

    coeffSumA.clear();
    coeffSumB.clear();


    double shift = combinedSpectrum.trueCenterFreq;
    for (double freq : combinedSpectrum.freqAxis) {
        exclusionLine.freqAxis.push_back(shift + freq);
        exclusionLine.powers.push_back(100);

        coeffSumA.push_back(0);
        coeffSumB.push_back(0);
    }
}



/**
 * @brief Move the 90% exclusion line to it's new coupling strengths based on updated information
 * 
 * @param combinedSpectrum combinedSpectrum object containing the data to update the exclusion cut with
 */
void BayesFactors::updateExclusionLine(CombinedSpectrum combinedSpectrum){
    if (coeffSumA.empty()) {
        init(combinedSpectrum);
    }

    double coeffA, coeffB;
    double scanFactor, newExcludedStrength;
    double fourLnPtOne = 9.210340372; // Numerical factor 4*ln(0.1) that goes into quadratic formula, 0.1 set by desried exclusion level (90%)

    for(int i=0; i<combinedSpectrum.powers.size(); i++){
        scanFactor = (1/sigmaProc)*sqrt(combinedSpectrum.weightSum[i]);

        coeffSumA[startIndex+i] += scanFactor*scanFactor/2;
        coeffSumB[startIndex+i] += scanFactor*combinedSpectrum.powers[i]/combinedSpectrum.sigmaCombined[i];

        coeffA = coeffSumA[startIndex+i];
        coeffB = coeffSumB[startIndex+i];

        newExcludedStrength = (coeffB+std::sqrt(coeffB*coeffB+fourLnPtOne*coeffA))/(2*coeffA);

        exclusionLine.powers[startIndex+i] = newExcludedStrength;
    }
}
