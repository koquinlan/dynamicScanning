#include "matplotlibcpp.h"
namespace plt = matplotlibcpp;

#include <iostream>
#include <cmath>

#include <string>
#include <vector>

#include <chrono>
#include <thread>
#include <stdexcept>
#include <fftw3.h>

#include "PSG.hpp"
#include "AWG.hpp"
#include "ATS.hpp"
#include "tests.hpp"

int main() {
    printAvailableResources();

    // psgTesting(21);
    // awgTesting(10);

    try{
        // Try to import an FFTW plan if available
        const char* wisdomFilePath = "fftw_wisdom.txt";
        if (fftw_import_wisdom_from_filename(wisdomFilePath) != 0) {
            std::cout << "Successfully imported FFTW wisdom from file." << std::endl;
        }
        else {
            std::cout << "Failed to import FFTW wisdom from file." << std::endl;
        }

        ATS alazarCard(1, 1);

        alazarCard.setAcquisitionParameters((U32)32e6, (U32)16e7, 0);

        fftw_complex* rawData = alazarCard.AcquireData();

        // Create an FFTW plan
        int N = (int)alazarCard.acquisitionParams.samplesPerAcquisition;

        std::cout << "Creating plan for N = " << std::to_string(N) << std::endl;
        fftw_complex* fftwInput = reinterpret_cast<fftw_complex*>(fftw_malloc(sizeof(fftw_complex) * N));
        fftw_complex* fftwOutput = reinterpret_cast<fftw_complex*>(fftw_malloc(sizeof(fftw_complex) * N));
        fftw_plan plan = fftw_plan_dft_1d(N, fftwInput, fftwOutput, FFTW_FORWARD, FFTW_ESTIMATE);
        std::cout << "Plan created!" << std::endl;

        fftw_complex* procData = processDataFFT(rawData, plan, N);

        // Plot the real and imaginary components of the FFT output
        std::vector<double> freq(N);
        std::vector<double> fftVoltage(N);
        std::vector<double> fftPower(N);

        for (int i = 0; i < N; ++i) {
            freq[i] = (static_cast<double>(i)-static_cast<double>(N)/2)*alazarCard.acquisitionParams.sampleRate/N;
            fftVoltage[i] = std::sqrt((procData[i][0]*procData[i][0] + procData[i][1]*procData[i][1]))/(double)N;

            fftPower[i] = fftVoltage[i]*fftVoltage[i]/alazarCard.acquisitionParams.inputImpedance;
        }

        plt::plot(freq, fftVoltage, "b");
        plt::plot(freq, fftPower, "r");
        plt::show();



        // int nShots = 4;
        // for (int i=0; i < nShots; i++){
        //     std::pair<std::vector<unsigned short>, std::vector<unsigned short>> rawData = alazarCard.AcquireData();
        //     processDataFFT(rawData, alazarCard.acquisitionParams, plan, fftwInput);
        // }


        // Save the plan using FFTW wisdom
        fftw_export_wisdom_to_filename(wisdomFilePath);
        std::cout << "FFTW wisdom saved to file." << std::endl;

        // Cleanup
        fftw_destroy_plan(plan);
        fftw_free(fftwInput);
        fftw_free(fftwOutput);

        /*
        std::pair<std::vector<double>, std::vector<double>> procData = processData(rawData, alazarCard.acquisitionParams);
        std::vector<double> channelDataA = procData.first;
        std::vector<double> channelDataB = procData.second;

        plt::plot(channelDataA);
        plt::plot(channelDataB);
        plt::show();
        */
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }

    return 0;
}
