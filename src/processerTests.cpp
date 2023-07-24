/**
 * @file processerTests.cpp
 * @author Kyle Quinlan (kyle.quinlan@colorado.edu)
 * @brief 
 * @version 0.1
 * @date 2023-07-18
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include "decs.hpp"

int main() {
    double sampleRate = 30e6;
    double cutoffFrequency = 10e3;

    std::string filename = "../../../src/dataProcessing/raw_data_probe_1.csv";
    std::vector<std::vector<double>> rawData = readCSV(filename, 10);

    std::cout << "Read " << rawData.size() << " spectra from " << filename << "\n" << std::endl;

    DataProcessor proc;

    for (int i = 0; i < rawData.size(); i++) {
        proc.addRawSpectrumToBaseline(rawData[i]);
    }


    std::vector<double> filteredRunningAverage = proc.runningAverage;
    double* averagedData[1];
    averagedData[0] = filteredRunningAverage.data();

    Dsp::FilterDesign <Dsp::ChebyshevII::Design::LowPass<3>, 1> f;
    Dsp::Params params;
    params[0] = sampleRate; // sample rate
    params[1] = 3;    // order
    params[2] = cutoffFrequency; // cutoff frequency
    params[3] = 30;  // stopband attenuation

    /** Useful to watch in debug mode to learn about filter **/
    // f.getKind();
    // f.getName();
    // f.getNumParams();
    // f.getParamInfo(0);
    // f.getParamInfo(1);
    // f.getParamInfo(2);

    f.setParams(params);
    f.process((int) proc.runningAverage.size(), averagedData); // Pass the pointer to the vector's data

    std::reverse(filteredRunningAverage.begin(), filteredRunningAverage.end());
    f.process((int) proc.runningAverage.size(), averagedData);
    std::reverse(filteredRunningAverage.begin(), filteredRunningAverage.end());

    plt::plot(rawData[0]);
    plt::plot(proc.runningAverage);
    plt::plot(filteredRunningAverage);
    plt::show();


    // Create an array of frequency points for the frequency response plot
    int numPoints = 10000; // You can adjust this based on the desired resolution
    std::vector<double> freqPoints(numPoints);
    std::vector<std::complex<double>> response(numPoints);

    std::vector<double> magnitude(numPoints);
    std::vector<double> phase(numPoints);
    for (int i = 0; i < numPoints; i++) {
        freqPoints[i] = 100 * static_cast<double>(i) / (numPoints - 1);
        
        response[i] = f.response(freqPoints[i]*(cutoffFrequency/sampleRate));

        magnitude[i] = std::abs(response[i]);
        phase[i] = std::arg(response[i]);
    }

    unwrapPhase(phase);


    // Create separate figures for magnitude and phase plots
    plt::figure();
    plt::loglog(freqPoints, magnitude);
    plt::title("Magnitude Response");
    plt::xlabel("Normalized Frequency");
    plt::ylabel("Magnitude");
    plt::grid(true);

    plt::figure();
    plt::semilogx(freqPoints, phase);
    plt::title("Phase Response");
    plt::xlabel("Normalized Frequency");
    plt::ylabel("Phase (radians)");
    plt::grid(true);

    plt::show();


    return 0;
}





/**
std::vector<double> savitzkyGolayFilter(const std::vector<double>& data, int windowSize, int polynomialDegree) {
    int n = data.size();
    int halfWindowSize = windowSize / 2;
    int maxOrder = polynomialDegree + 1;

    std::vector<double> filteredData(n);

    for (int i = 0; i < n; ++i) {
        int startIdx = max(0, i - halfWindowSize);
        int endIdx = min(n - 1, i + halfWindowSize);
        int numRows = endIdx - startIdx + 1;

        // Initialize the design matrix and the target vector
        std::vector<std::vector<double>> A(numRows, std::vector<double>(maxOrder, 0.0));
        std::vector<double> b(numRows, 0.0);

        // Populate the design matrix and the target vector
        for (int j = startIdx; j <= endIdx; ++j) {
            for (int k = 0; k < maxOrder; ++k) {
                A[j - startIdx][k] = pow(j - i, k);
            }
            b[j - startIdx] = data[j];
        }

        // Perform the Gram-Schmidt orthogonalization
        for (int j = 0; j < maxOrder; ++j) {
            for (int k = 0; k < j; ++k) {
                double dotProduct = 0.0;
                for (int l = 0; l < numRows; ++l) {
                    dotProduct += A[l][j] * A[l][k];
                }
                for (int l = 0; l < numRows; ++l) {
                    A[l][j] -= dotProduct * A[l][k];
                }
            }
            double norm = 0.0;
            for (int l = 0; l < numRows; ++l) {
                norm += A[l][j] * A[l][j];
            }
            norm = sqrt(norm);
            if (norm > 1e-12) {
                for (int l = 0; l < numRows; ++l) {
                    A[l][j] /= norm;
                }
            }
        }

        // Solve the least squares problem to obtain the polynomial coefficients
        std::vector<double> x(maxOrder, 0.0);
        for (int j = 0; j < maxOrder; ++j) {
            for (int k = 0; k < numRows; ++k) {
                x[j] += A[k][j] * b[k];
            }
        }

        // Compute the filtered value at the center index
        filteredData[i] = x[polynomialDegree];
    }

    return filteredData;
} 
 
**/