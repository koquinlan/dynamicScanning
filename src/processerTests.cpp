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

int main(){
    std::string filename = "../../../src/dataProcessing/raw_data_probe_1.csv";
    std::vector<std::vector<double>> rawData = readCSV(filename);

    
    DataProcessor proc;

    for (int i = 0; i < rawData.size(); i++) {
        proc.addRawSpectrumToBaseline(rawData[i]);
    }
    

    plt::plot(rawData[0]);
    plt::plot(proc.runningAverage);
    plt::show();

    return 0;
}