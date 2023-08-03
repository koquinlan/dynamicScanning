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


std::tuple<double, double> vectorStats(const std::vector<double>& vec) {
    if (vec.empty()) {
        // Return NaN to indicate that the mean is undefined for an empty vector.
        return std::make_tuple(std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::quiet_NaN());
    }

    double sum = std::accumulate(vec.begin(), vec.end(), 0.0);
    double mean = sum / static_cast<double>(vec.size());

    double sumSquaredDiff = 0.0;
    for (const double& value : vec) {
        double diff = value - mean;
        sumSquaredDiff += diff * diff;
    }

    double variance = sumSquaredDiff / static_cast<double>(vec.size());
    return std::make_tuple(mean, std::sqrt(variance));
}


void trimVector(std::vector<double>& vec, double cutPercentage) {
    if (vec.size() < 3) {
        // The vector is too small to remove elements.
        return;
    }

    // Calculate the number of elements to be removed from the front and back.
    int numElementsToRemove = (int) std::round(vec.size() * cutPercentage);

    // Erase elements from the beginning and end of the vector.
    vec.erase(vec.begin(), vec.begin() + numElementsToRemove);
    vec.erase(vec.end() - numElementsToRemove, vec.end());
}



std::vector<int> findOutliers(const std::vector<double>& data, int windowSize, double multiplier) {
    int halfWindow = windowSize / 2;
    std::vector<int> outliers;

    for (int i = halfWindow; i < data.size() - halfWindow; ++i) {
        std::vector<double> windowData(data.begin() + i - halfWindow, data.begin() + i + halfWindow + 1);
        
        double mean, stdDev;
        std::tie(mean, stdDev) = vectorStats(windowData);

        // Check if the current element is an outlier
        if (data[i] > (mean + multiplier * stdDev)) {
            outliers.push_back(i);
        }
    }

    return outliers;
}



int findClosestIndex(std::vector<double> vec, double target) {
    double minDifference = std::abs(vec[0] - target);
    int closestIndex = 0;

    for (int i = 1; i < vec.size(); ++i) {
        double difference = std::abs(vec[i] - target);
        if (difference < minDifference) {
            minDifference = difference;
            closestIndex = i;
        }
    }

    return closestIndex;
}



int findMaxIndex(std::vector<double> vec, int startIndex, int endIndex) {
    if (startIndex < 0 || endIndex > vec.size()) {
        // Handle invalid input window
        return -1;
    }

    int maxIndex = startIndex;
    double maxValue = vec[startIndex];

    for (int i = startIndex + 1; i < endIndex; ++i) {
        if (vec[i] > maxValue) {
            maxValue = vec[i];
            maxIndex = i;
        }
    }

    return maxIndex;
}