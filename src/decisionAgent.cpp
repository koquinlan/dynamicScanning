/**
 * @file decisionAgent.cpp
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2023-09-14
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include "decs.hpp"

void DecisionAgent::trimSNRtoMatch(Spectrum spectrum) {
    int startIndex=0;
    while (SNR.freqAxis[startIndex+1] < spectrum.freqAxis[0]) {
        startIndex++;
    }

    trimmedSNR.powers.clear();
    trimmedSNR.freqAxis.clear();

    trimmedSNR.powers.resize(spectrum.freqAxis.size());
    trimmedSNR.freqAxis.resize(spectrum.freqAxis.size());

    for(int i=0; i < trimmedSNR.powers.size(); i++){
        trimmedSNR.powers[i] = SNR.powers[i+startIndex];
        trimmedSNR.freqAxis[i] = SNR.freqAxis[i+startIndex];
    }
}


void DecisionAgent::setTargets(){
    double SNRsum=0;
    for (int i=0; i<trimmedSNR.powers.size(); i++){
        SNRsum += trimmedSNR.powers[i]*trimmedSNR.powers[i];
        inProgressTargets.push_back(0);
    }

    double cumSum=0;
    for (int i=(int)(inProgressTargets.size())-1; i>=0; i--){
        cumSum += trimmedSNR.powers[i]*trimmedSNR.powers[i];
        inProgressTargets[i] = (std::sqrt(SNRsum/cumSum)*targetCoupling - targetCoupling)/2 + targetCoupling; 
    }
}


void DecisionAgent::setPoints(){
    /** OPTION FOUR - reward for center 70% with inverse scaled square SNR **/
    double SNRsum=0;
    double cumSum=0;

    for (int i=0; i<trimmedSNR.powers.size(); i++){
        SNRsum += trimmedSNR.powers[i]*trimmedSNR.powers[i];
    }

    int j = trimmedSNR.powers.size()-1;
    while(cumSum/SNRsum < 0.3){
        cumSum += trimmedSNR.powers[j]*trimmedSNR.powers[j];
        points[j] = 0;
        j--;
    }


    double norm = trimmedSNR.powers[j]*trimmedSNR.powers[j];
    while(cumSum/SNRsum < 0.7){
        cumSum += trimmedSNR.powers[j]*trimmedSNR.powers[j];

        points[j] = points[j+1] + trimmedSNR.powers[j]*trimmedSNR.powers[j]/norm;
        j--;
    }

    while(j >= 0){
        points[j]=0;
        j--;
    }
}


int DecisionAgent::getDecision(std::vector<double> activeExclusionLine){
    return (checkScore(activeExclusionLine) <= threshold);
}


double DecisionAgent::checkScore(std::vector<double> activeExclusionLine){
    double score = 0;

    for (int i=0; i < activeExclusionLine.size(); i++){
        if (activeExclusionLine[i] > inProgressTargets[i]){
            score += points[i]*(activeExclusionLine[i]/inProgressTargets[i])*(activeExclusionLine[i]/inProgressTargets[i]);
        } 
    }
    
    return score;
}