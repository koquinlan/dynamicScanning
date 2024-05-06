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


    std::size_t matchingIndex = 1;
    for(std::size_t i=0; i<trimmedSNR.powers.size(); i++){
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
    for (std::size_t i=0; i<trimmedSNR.powers.size(); i++){
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

    for (std::size_t i=0; i<trimmedSNR.powers.size(); i++){
        SNRsum += trimmedSNR.powers[i]*trimmedSNR.powers[i];
        points.push_back(0);
    }

    int j = (int)trimmedSNR.powers.size()-1;
    while(cumSum/SNRsum < 0.4){
        cumSum += trimmedSNR.powers[j]*trimmedSNR.powers[j];
        points[j] = 0;
        j--;
    }

    while(cumSum/SNRsum < 0.85){
        cumSum += trimmedSNR.powers[j]*trimmedSNR.powers[j];

        points[j] = cumSum/SNRsum;
        j--;
    }

    while(j >= 0){
        points[j]=0;
        j--;
    }

    // Normalize point to the max value
    double maxPoint = *std::max_element(points.begin(), points.end());
    for (std::size_t i=0; i<points.size(); i++){
        points[i] = points[i]/maxPoint;
    }
}


int DecisionAgent::getDecision(std::vector<double> activeExclusionLine, int numShots){
    if (decisionMaking && (numShots > (minSpectra - 3))){ // 3 spectra to account for decision delay
        return (checkScore(activeExclusionLine) <= threshold);
    } else {
        return 0;
    }
}


double DecisionAgent::checkScore(std::vector<double> activeExclusionLine){
    double score = 0;

    for (std::size_t i=0; i < activeExclusionLine.size(); i++){
        if (activeExclusionLine[i] > inProgressTargets[i]){
            score += points[i]*(activeExclusionLine[i]/inProgressTargets[i])*(activeExclusionLine[i]/inProgressTargets[i]);
        } 
    }
    
    return score;
}



void DecisionAgent::toggleDecisionMaking(bool decisionMaking) {
    this->decisionMaking = decisionMaking;
}



void DecisionAgent::saveState(std::string statePath) {
    saveVector(trimmedSNR.powers, statePath + "trimmedSNR.csv");
    
    saveVector(points, statePath + "points.csv");
    saveVector(inProgressTargets, statePath + "inProgressTargets.csv");
}