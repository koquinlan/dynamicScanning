/**
 * @file decisionAgent.hpp
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2023-09-14
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#ifndef DECISION_H
#define DECISION_H

#include "decs.hpp"

class DecisionAgent {
public:
    Spectrum SNR, trimmedSNR;

    double targetCoupling;
    std::vector<double> inProgressTargets, points;

    double threshold = 0;


    void resizeSNRtoMatch(Spectrum spectrum);
    void resizeSNRtoMatch(CombinedSpectrum spectrum);
    void setTargets();

    int getDecision(std::vector<double> activeExclusionLine);
    double checkScore(std::vector<double> activeExclusionLine);
    void setPoints();

    void toggleDecisionMaking(int decisionMaking);

private:
    int decisionMaking = 1;
};

#endif // DECISION_H