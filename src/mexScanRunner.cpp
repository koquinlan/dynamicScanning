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

#include "utils/json.hpp"
using json = nlohmann::json;

#include "decs.hpp"

struct TopLevelParameters {
    bool decisionMaking;
    std::string baselinePath;
    std::string statePath;
    std::string savePath;
};

struct DataParameters {
    double maxIntegrationTime;
    double sampleRate;
    double RBW;
    double trueCenterFreq;
    int subSpectraAveragingNumber;
};

struct FilterParameters {
    double cutoffFrequency;
    int poleNumber;
    double stopbandAttenuation;
};

struct ScanParameters {
    TopLevelParameters topLevelParameters;
    DataParameters dataParameters;
    FilterParameters filterParameters;
};


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
        ScanParameters scanParameters = unpackScanParameters(inputs[0]);

        matlab::data::ArrayFactory factory;
        matlab::data::TypedArray<double> resultArray = factory.createArray<double>({1, 1}, {scanParameters.dataParameters.sampleRate});

        outputs[0] = std::move(resultArray);
    }

private:
    ScanParameters unpackScanParameters(matlab::data::CharArray const& inputJson) {
        ScanParameters scanParameters;
        json inputParams = json::parse(inputJson);

        scanParameters.topLevelParameters.decisionMaking = inputParams["topLevelParams"]["decisionMaking"];
        scanParameters.topLevelParameters.baselinePath = inputParams["topLevelParams"]["baselinePath"];
        scanParameters.topLevelParameters.statePath = inputParams["topLevelParams"]["statePath"];
        scanParameters.topLevelParameters.savePath = inputParams["topLevelParams"]["savePath"];

        scanParameters.dataParameters.maxIntegrationTime = inputParams["dataParams"]["maxIntegrationTime"];
        scanParameters.dataParameters.sampleRate = inputParams["dataParams"]["sampleRate"];
        scanParameters.dataParameters.RBW = inputParams["dataParams"]["RBW"];
        scanParameters.dataParameters.trueCenterFreq = inputParams["dataParams"]["trueCenterFreq"];
        scanParameters.dataParameters.subSpectraAveragingNumber = inputParams["dataParams"]["subSpectraAveragingNumber"];

        scanParameters.filterParameters.cutoffFrequency = inputParams["filterParams"]["cutoffFrequency"];
        scanParameters.filterParameters.poleNumber = inputParams["filterParams"]["poleNumber"];
        scanParameters.filterParameters.stopbandAttenuation = inputParams["filterParams"]["stopbandAttenuation"];
    
        return scanParameters;
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
        if (inputs.size() != 1 || inputs[0].getType() != matlab::data::ArrayType::CHAR) {
            throw std::invalid_argument("Expected a single input of type char.");
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

