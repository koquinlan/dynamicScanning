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
        matlab::data::CharArray const& baselineJson = inputs[1];
        json baselineParams = json::parse(baselineJson);

        // Acquire baseline data
        ScanRunner scanRunner(scanParameters);
        scanRunner.refreshBaselineAndBadBins(baselineParams["repeats"], baselineParams["subSpectra"], baselineParams["savePlots"]);


        matlab::data::ArrayFactory factory;
        matlab::data::TypedArray<double> resultArray = factory.createArray<double>({1, 1}, {scanParameters.dataParameters.sampleRate});

        outputs[0] = std::move(resultArray);
    }

private:
    ScanParameters readInput(matlab::data::CharArray const& inputJson) {
        return unpackScanParameters(json::parse(inputJson));
    }


    void checkArguments(matlab::mex::ArgumentList outputs, matlab::mex::ArgumentList inputs) {
        if (inputs.size() != 2){
            throw std::invalid_argument("Two inputs required.");
        }

        if (inputs[0].getType() != matlab::data::ArrayType::CHAR) {
            throw std::invalid_argument("Expected a first input of type char - json string.");
        }

        if (inputs[1].getType() != matlab::data::ArrayType::CHAR) {
            throw std::invalid_argument("Expected a second input of type char - json string.");
        }

        if (outputs.size() != 1) {
            throw std::invalid_argument("One output required.");
        }
    }
};