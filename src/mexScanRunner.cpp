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

class MexFunction : public matlab::mex::Function {
public:
    void operator()(matlab::mex::ArgumentList outputs, matlab::mex::ArgumentList inputs) {
        // Confirm valid inputs
        checkArguments(outputs, inputs);

        // Convert input array to std::vector
        matlab::data::Array inputArray = std::move(inputs[0]);
        std::vector<double> inputVector = convertToVector<double>(inputArray);

        // Perform some operation
        for (auto& num : inputVector) {
            num *= 3;
        }

        matlab::data::ArrayFactory factory;
        matlab::data::TypedArray<double> resultArray = factory.createArray<std::vector<double>::iterator>(
            {1, inputVector.size()},
            inputVector.begin(),
            inputVector.end()       
        );
        
        outputs[0] = std::move(resultArray);
    }

private:
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
        if (inputs.size() != 1) {
            throw std::invalid_argument("One input required.");
        }
        if (outputs.size() != 1) {
            throw std::invalid_argument("One output required.");
        }
        if (inputs[0].getType() != matlab::data::ArrayType::DOUBLE ||
            inputs[0].getType() == matlab::data::ArrayType::COMPLEX_DOUBLE) {
            throw std::invalid_argument("Input must be a real double array.");
        }
    }
};