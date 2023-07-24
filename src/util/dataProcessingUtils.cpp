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
        if (diff > M_PI) {
            phase[i] -= 2 * M_PI;
        } else if (diff < -M_PI) {
            phase[i] += 2 * M_PI;
        }
    }
}