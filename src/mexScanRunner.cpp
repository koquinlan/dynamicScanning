/**
 * @file mexScanRunner.cpp
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2024-02-13
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#include "mex.hpp"
#include "mexAdapter.hpp"

#include "decs.hpp"

/**
 * @brief
 * 
*/
class MexFunction : public matlab::mex::Function {
public:
    void operator()(matlab::mex::ArgumentList outputs, matlab::mex::ArgumentList inputs) {
        // Confirm valid inputs
        checkArguments(outputs, inputs);

        // Unpack scan parameters into struct
        ScanParameters scanParameters = readInput(inputs[0]);
        bool fullSave = readBooleanInput(inputs[1]);

        // Begin scanning
        ScanRunner scanRunner(scanParameters);

        std::ifstream file(scanParameters.topLevelParameters.statePath + "scanInfo.json");
        if (file.is_open()) {
            file.close();
            scanRunner.loadState();
        }
        else { file.close(); }
        
        scanRunner.acquireData();
        if (fullSave) { scanRunner.saveData(); }
        scanRunner.saveState();
        

        // Return performance data via JSON string
        std::string jsonString = performanceToJson().dump();
    
        matlab::data::ArrayFactory factory;
        matlab::data::CharArray resultString = factory.createCharArray(jsonString);
        
        outputs[0] = std::move(resultString);
    }

private:
    ScanParameters readInput(matlab::data::CharArray const& inputJson) {
        return unpackScanParameters(json::parse(inputJson));
    }


    bool readBooleanInput(matlab::data::Array const& input) {
        matlab::data::TypedArray<bool> inputData = std::move(input);
        return inputData[0];
    }


    template<typename T>
    std::vector<T> convertToVector(matlab::data::Array const& input) {
        std::vector<T> output;
        matlab::data::TypedArray<T> inputData = std::move(input);

        for (auto element : inputData) {
            output.push_back(element);
        }

        return output;
    }


    void checkArguments(matlab::mex::ArgumentList outputs, matlab::mex::ArgumentList inputs) {
        if (inputs.size() != 2) {
            throw std::invalid_argument("Expected exactly two inputs - scan parameters and boolean flag for full save.");
        }
        if (inputs[0].getType() != matlab::data::ArrayType::CHAR) {
            throw std::invalid_argument("First input must be of type char (JSON string).");
        }
        if (inputs[1].getType() != matlab::data::ArrayType::LOGICAL) {
            throw std::invalid_argument("Second input must be a logical scalar (boolean).");
        }
        if (outputs.size() != 1) {
            throw std::invalid_argument("One output required.");
        }
    }
};



// /**
//  * @file threadedTesting.cpp
//  * @author Kyle Quinlan (kyle.quinlan@colorado.edu)
//  * @brief For rapid prototyping and testing of multithreaded control and acquisition code. See src/controlTests.cpp for single threaded development.
//  * @version 0.1
//  * @date 2023-06-30
//  * 
//  * @copyright Copyright (c) 2023
//  * 
//  */
// #include "decs.hpp"

// #define REFRESH_PROCESSOR (0)


// void dynamicRun(int maxSpectraPerStep, int minSpectraPerStep, int subSpectraAveragingNumber, double stepSize, int numSteps, double targetCoupling);
// void staticRun(int maxSpectraPerStep, int subSpectraAveragingNumber, double stepSize, int numSteps, double targetCoupling);

// int main() {
//     int steps(500), subSpectraAveragingNumber(15);
//     double couplingTarget(3.4e-5), stepSize(0.1);

//     // int maxSpectraPerStep, int minSpectraPerStep, int subSpectraAveragingNumber, double stepSize, int numSteps, double targetCoupling
//     // try{    
//     //     dynamicRun(75, 45, subSpectraAveragingNumber, stepSize, steps, couplingTarget);
//     // }
//     // catch(const std::exception& e)
//     // {
//     //     std::cerr << e.what() << '\n';
//     // }


//     int maxSpectras[] = {48,49,50};
//     // int maxSpectraPerStep, int subSpectraAveragingNumber, double stepSize, int numSteps, double targetCoupling
//     for (int maxSpectra : maxSpectras) {
//         staticRun(maxSpectra, subSpectraAveragingNumber, stepSize, steps, couplingTarget);
//     }

//     return 0;
// }


// void dynamicRun(int maxSpectraPerStep, int minSpectraPerStep, int subSpectraAveragingNumber, double stepSize, int numSteps, double targetCoupling){
//     double maxIntegrationTime = maxSpectraPerStep*subSpectraAveragingNumber*0.01; // seconds

//     ScanRunner scanRunner(maxIntegrationTime, 0, 1);
//     scanRunner.subSpectraAveragingNumber = subSpectraAveragingNumber;
//     scanRunner.setTarget(targetCoupling);
//     scanRunner.decisionAgent.minShots = minSpectraPerStep;


//     #if REFRESH_PROCESSOR
//     scanRunner.refreshBaselineAndBadBins(1, 32, 1);
//     #endif

//     scanRunner.acquireData();
//     for (int i = 0; i < numSteps; i++) {
//         scanRunner.step(stepSize);
//         scanRunner.acquireData();
//     }

//     scanRunner.saveData(1);
// }


// void staticRun(int maxSpectraPerStep, int subSpectraAveragingNumber, double stepSize, int numSteps, double targetCoupling){
//     double maxIntegrationTime = maxSpectraPerStep*subSpectraAveragingNumber*0.01; // seconds

//     ScanRunner scanRunner(maxIntegrationTime, 0, 0);
//     scanRunner.subSpectraAveragingNumber = subSpectraAveragingNumber;
//     scanRunner.setTarget(targetCoupling);

//     #if REFRESH_PROCESSOR
//     scanRunner.refreshBaselineAndBadBins(1, 32, 1);
//     #endif

//     scanRunner.acquireData();
//     for (int i = 0; i < numSteps; i++) {
//         std::cout << "Stepping: i=" << std::to_string(i) << std::endl;
//         scanRunner.step(stepSize);
//         std::cout << "Acquiring Data" << std::endl;
//         scanRunner.acquireData();

//         if (i % 50 == 0) {
//             std::cout << "Saving Data" << std::endl;
//             scanRunner.saveData();
//         }
//     }

//     std::cout << "Saving Data" << std::endl;
//     scanRunner.saveData();
// }

