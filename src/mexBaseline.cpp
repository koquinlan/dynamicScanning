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

        // Begin scanning
        ScanRunner scanRunner(scanParameters);


        matlab::data::ArrayFactory factory;
        matlab::data::TypedArray<double> resultArray = factory.createArray<double>({1, 1}, {scanParameters.dataParameters.sampleRate});

        outputs[0] = std::move(resultArray);
    }

private:
    ScanParameters readInput(matlab::data::CharArray const& inputJson) {
        return unpackScanParameters(json::parse(inputJson));
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