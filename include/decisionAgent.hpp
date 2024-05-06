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

    double threshold = 1.5;
    int minSpectra;


    void resizeSNRtoMatch(Spectrum spectrum);
    void resizeSNRtoMatch(CombinedSpectrum spectrum);
    void setTargets();

    int getDecision(std::vector<double> activeExclusionLine, int numShots);
    double checkScore(std::vector<double> activeExclusionLine);
    void setPoints();

    void toggleDecisionMaking(bool decisionMaking);

    void saveState(std::string statePath);

private:
    bool decisionMaking = true;
};

#endif // DECISION_H