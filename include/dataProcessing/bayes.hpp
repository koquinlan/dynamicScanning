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
    int cutoffIndex=0;
    double freqRes=0;
    
    double sigmaProc=0.1;

    // coefficients in the quadratic formula for the 90% excluded coupling strength
    std::vector<double> coeffSumA, coeffSumB;

    Spectrum exclusionLine;
};