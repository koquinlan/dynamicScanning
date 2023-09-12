#include "decs.hpp"

/**
 * @brief Wrapper class to simulate, manipulate and store simulated scanning data. Contains functionality to calculate either an 
 * aggregate update or a 90% exclusion strength
 * 
 */
class BayesFactors{
    public:

    void init(CombinedSpectrum combinedSpectrum);
    void updateExclusionLine(CombinedSpectrum combinedSpectrum);

    void step(double stepSize);


    // private:
    int startIndex=0;
    double freqRes=0;
    
    double sigmaProc=0.1;

    // coefficients in the quadratic formula for the 90% excluded coupling strength
    std::vector<double> coeffSumA, coeffSumB;

    Spectrum exclusionLine;
};

//     Processor proc;
    
//     std::vector<double> fullFreqRange;
//     std::vector<double> freqRange;

//     // Bayes factor/exclusion line info
//     double couplingStrength, eta;
//     std::vector<double> aggregateUpdate;
//     std::vector<double> exclusionLine;

//     // coefficients in the quadratic formula for the 90% excluded coupling strength
//     std::vector<double> coeffSumA; 
//     std::vector<double> coeffSumB;
    
//     // Reward parameters
//     int rewardStartIndex, rewardEndIndex;
//     double targetCoupling;
//     // Piecewise reward
//     double belowThresholdValue, aboveThresholdValue, stepPunish;
//     // Tanh reward
//     double peakSharpness, peakReward;
    

//     // Axion generation options
//     double gGammaSim, gGammaFactors, axionFreq;
//     int axionBool;

//     // Gain profile options
//     double maxGain, gammaGain;
//     std::vector<double> SNR;

//     // Scanning options
//     double centerFreq, sigmaProc;
//     double tuningStepFreq, scanningWindowSize;
//     int freqWindowSize, freqRes, nSpectra;

//     int startIndex=0;


//     void init(const std::string& parametersFile);
//     void step(int stepForward);

//     Feature getState(int normalized = 1) const;
//     Feature getStateAxis() const;
//     std::vector<double> getActiveAxis() const;
//     double getReward() const;
//     double getScore() const;

    
//     Feature windowAverage(const std::vector<double>& input, int outputSize) const;
//     Feature windowMax(const std::vector<double>& input, int outputSize) const;

//     private:

//     Feature state;

//     void initParams(const std::string& parametersFile);

//     combinedSpectrum genCombined();

//     void updateExclusionLine(combinedSpectrum combined);
//     void updateState();
//     double rewardFunction(double excludedCoupling) const;
// };