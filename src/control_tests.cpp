#include "matplotlibcpp.h"
namespace plt = matplotlibcpp;

#include <iostream>
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

        alazarCard.setAcquisitionParameters((U32)32e6, (U32)32e6, 0);

        std::pair<std::vector<unsigned short>, std::vector<unsigned short>> rawData = alazarCard.AcquireData();

        // Create an FFTW plan
        int N = (int)rawData.first.size();
        fftw_complex* fftwInput = reinterpret_cast<fftw_complex*>(fftw_malloc(sizeof(fftw_complex) * N));
        fftw_plan plan = fftw_plan_dft_1d(N, fftwInput, fftwInput, FFTW_FORWARD, FFTW_MEASURE);

        processDataFFT(rawData, alazarCard.acquisitionParams, plan, fftwInput);

        // Plot the real and imaginary components of the FFT output
        std::vector<double> freq(N);
        std::vector<double> fftMag(N);

        for (int i = 0; i < N; ++i) {
            freq[i] = static_cast<double>(i);
            fftMag[i] = fftwInput[i][0]*fftwInput[i][0] + fftwInput[i][1]*fftwInput[i][1];
        }

        plt::plot(freq, fftMag, "b");
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
