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
    // Add some toy data to work with
    std::string filename = "../../../src/dataProcessing/raw_data_probe_1.csv";
    std::vector<std::vector<double>> rawData = readCSV(filename, 10);

    std::cout << "Read " << rawData.size() << " spectra from " << filename << "\n" << std::endl;

    DataProcessor proc;

    for (int i = 0; i < rawData.size(); i++) {
        proc.addRawSpectrumToRunningAverage(rawData[i]);
    }


    // Apply filtering to the running average
    proc.setFilterParams(30e6, 3, 20e3, 30);
    proc.updateBaseline();


    // Display results
    proc.displayFilterResponse();

    plt::figure();
    plt::plot(rawData[0]);
    plt::plot(proc.runningAverage);
    plt::plot(proc.currentBaseline);

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