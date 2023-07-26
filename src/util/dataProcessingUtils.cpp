/**
 * @file dataProcessingUtils.cpp
 * @author Kyle Quinlan (kyle.quinlan@colorado.edu)
 * @brief 
 * @version 0.1
 * @date 2023-07-24
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include "decs.hpp"

void unwrapPhase(std::vector<double>& phase) {
    for (size_t i = 1; i < phase.size(); ++i) {
        double diff = phase[i] - phase[i - 1];
        if (diff > 4*M_PI/5) {
            phase[i] -= std::round(diff/M_PI) * M_PI;
        } else if (diff < -4*M_PI/5) {
            phase[i] += std::round(diff/M_PI) * M_PI;
        }
    }
}