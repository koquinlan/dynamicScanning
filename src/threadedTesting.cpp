#include "matplotlibcpp.h"
namespace plt = matplotlibcpp;

#include <iostream>
#include <string>
#include <vector>

#include <chrono>
#include <thread>
#include <stdexcept>
#include <future>

#include "PSG.hpp"
#include "AWG.hpp"
#include "ATS.hpp"
#include "tests.hpp"

int main() {
    std::chrono::seconds dura(5);
    std::this_thread::sleep_for(dura);

    int numShots = 5;
    double sampleRate = 32e4;
    double samplesPerAcquisition = 7.5e5;

    // Try to import an FFTW plan if available
    const char* wisdomFilePath = "fftw_wisdom.txt";
    if (fftw_import_wisdom_from_filename(wisdomFilePath) != 0) {
        std::cout << "Successfully imported FFTW wisdom from file." << std::endl;
    }
    else {
        std::cout << "Failed to import FFTW wisdom from file." << std::endl;
    }


    // Prepare for acquisition and create FFTW plan
    ATS alazarCard(1, 1);
    alazarCard.setAcquisitionParameters((U32)sampleRate, (U32)samplesPerAcquisition, 0);

    std::vector<std::future<fftw_complex*>> futures;

    int N = (int)alazarCard.acquisitionParams.samplesPerAcquisition;
    std::cout << "Creating plan for N = " << std::to_string(N) << std::endl;
    fftw_complex* fftwInput = reinterpret_cast<fftw_complex*>(fftw_malloc(sizeof(fftw_complex) * N));
    fftw_complex* fftwOutput = reinterpret_cast<fftw_complex*>(fftw_malloc(sizeof(fftw_complex) * N));
    fftw_plan plan = fftw_plan_dft_1d(N, fftwInput, fftwOutput, FFTW_FORWARD, FFTW_MEASURE);
    std::cout << "Plan created!" << std::endl;


    // Main acquisition and processing logic
    DWORD startTickCount = GetTickCount();
    double scanTime_sec = 0;
    for (int i = 0; i < numShots; i++) {
        DWORD scanTickCount = GetTickCount();
        fftw_complex* rawData = alazarCard.AcquireData();
        scanTime_sec += (GetTickCount() - scanTickCount) / 1000.;

        // Start a new async thread for processing each acquisition
        futures.push_back(std::async(std::launch::async, processDataFFT, rawData, plan, N));
    }

    // Wait for all asynchronous tasks to complete
    for (auto& future : futures) {
        fftw_complex* procData = future.get();
    }

    double fullTime_sec = (GetTickCount() - startTickCount) / 1000.;
    double expectedScanTime = (samplesPerAcquisition/sampleRate)*numShots;
    printf("\nMultithreaded run completed in %.3lf sec\n", fullTime_sec);
    printf("Time actively scanning %.3lf sec\n", scanTime_sec);
    printf("Expected scan time %.3lf sec\n", expectedScanTime);
    printf("Total live time efficiency: %.3lf \n\n", expectedScanTime/scanTime_sec);



    // Normal acquisition and processing logic
    startTickCount = GetTickCount();
    scanTime_sec = 0;
    for (int i = 0; i < numShots; i++) {
        DWORD scanTickCount = GetTickCount();
        fftw_complex* rawData = alazarCard.AcquireData();
        scanTime_sec += (GetTickCount() - scanTickCount) / 1000.;

        fftw_complex* procData = processDataFFT(rawData, plan, N);
    }

    fullTime_sec = (GetTickCount() - startTickCount) / 1000.;
    printf("\nSingle threaded run completed in %.3lf sec\n", fullTime_sec);
    printf("Time actively scanning %.3lf sec\n", scanTime_sec);
    printf("Expected scan time %.3lf sec\n", expectedScanTime);
    printf("Total live time efficiency: %.3lf \n\n", expectedScanTime/scanTime_sec);


    // Cleanup
    fftw_export_wisdom_to_filename(wisdomFilePath);
    std::cout << "FFTW wisdom saved to file." << std::endl;

    fftw_destroy_plan(plan);
    fftw_free(fftwInput);
    fftw_free(fftwOutput);

    return 0;
}
