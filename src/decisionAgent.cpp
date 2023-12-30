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

void DecisionAgent::resizeSNRtoMatch(CombinedSpectrum spectrum) {
    Spectrum temp;

    temp.powers = spectrum.powers;
    temp.freqAxis = spectrum.freqAxis;

    resizeSNRtoMatch(temp);
}




void DecisionAgent::resizeSNRtoMatch(Spectrum spectrum) {
    trimmedSNR.powers.clear();
    trimmedSNR.freqAxis.clear();

    trimmedSNR.powers.resize(spectrum.freqAxis.size());
    trimmedSNR.freqAxis.resize(spectrum.freqAxis.size());


    int matchingIndex = 1;
    for(int i=0; i<trimmedSNR.powers.size(); i++){
        while(spectrum.freqAxis[i] > SNR.freqAxis[matchingIndex]){
            matchingIndex++;

            if (matchingIndex >= SNR.freqAxis.size()){
                matchingIndex = int(SNR.freqAxis.size())-1;
                break;
            }
        }
        if (std::abs(SNR.freqAxis[matchingIndex]-spectrum.freqAxis[i]) > std::abs(SNR.freqAxis[matchingIndex-1]-spectrum.freqAxis[i])) {
            matchingIndex--;
        }

        trimmedSNR.powers[i] = SNR.powers[matchingIndex];
        trimmedSNR.freqAxis[i] = SNR.freqAxis[matchingIndex];
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
        inProgressTargets[i] = (std::sqrt(SNRsum/cumSum)*targetCoupling - targetCoupling)/1.05 + targetCoupling; 
    }
}


void DecisionAgent::setPoints(){
    /** OPTION FOUR - reward for center 40% with inverse scaled square SNR **/
    double SNRsum=0;
    double cumSum=0;

    for (int i=0; i<trimmedSNR.powers.size(); i++){
        SNRsum += trimmedSNR.powers[i]*trimmedSNR.powers[i];
        points.push_back(0);
    }

    int j = (int)trimmedSNR.powers.size()-1;
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


int DecisionAgent::getDecision(std::vector<double> activeExclusionLine, int numShots){
    if (decisionMaking && (numShots > minShots)){
        return (checkScore(activeExclusionLine) <= threshold);
    } else {
        return 0;
    }
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



void DecisionAgent::toggleDecisionMaking(int decisionMaking) {
    this->decisionMaking = decisionMaking;
}