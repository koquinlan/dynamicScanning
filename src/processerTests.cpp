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

    Dsp::FilterDesign<Dsp::Butterworth::Design::LowPass<4>, 1> f;
    Dsp::Params params;
    params[0] = 30e6; // sample rate
    params[1] = 4;    // order
    params[2] = 10e3; // cutoff frequency
    // params[3] = -30;  // stopband attenuation

    /** Useful to watch in debug mode to learn about filter **/
    // f.getKind();
    // f.getName();
    // f.getNumParams();
    // f.getParamInfo(0);
    // f.getParamInfo(1);
    // f.getParamInfo(2);

    f.setParams(params);
    f.process((int) proc.runningAverage.size(), averagedData); // Pass the pointer to the vector's data

    plt::plot(rawData[0]);
    plt::plot(proc.runningAverage);
    plt::plot(filteredRunningAverage);
    plt::show();

    return 0;
}
