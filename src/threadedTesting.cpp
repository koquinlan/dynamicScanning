/**
 * @file threadedTesting.cpp
 * @author Kyle Quinlan (kyle.quinlan@colorado.edu)
 * @brief For rapid prototyping and testing of multithreaded control and acquisition code. See src/controlTests.cpp for single threaded development.
 * @version 0.1
 * @date 2023-06-30
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include "decs.hpp"

int main() {
    // std::chrono::seconds dura(5);
    // std::this_thread::sleep_for(dura);

    double sampleRate = 32e6;
    double samplesPerAcquisition = 7.5e7;

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
    std::cout << "Acquisition parameters set. Collecting " << std::to_string(alazarCard.acquisitionParams.buffersPerAcquisition) << " buffers." << std::endl;


    int N = (int)alazarCard.acquisitionParams.samplesPerBuffer;
    std::cout << "Creating plan for N = " << std::to_string(N) << std::endl;
    fftw_complex* fftwInput = reinterpret_cast<fftw_complex*>(fftw_malloc(sizeof(fftw_complex) * N));
    fftw_complex* fftwOutput = reinterpret_cast<fftw_complex*>(fftw_malloc(sizeof(fftw_complex) * N));
    fftw_plan plan = fftw_plan_dft_1d(N, fftwInput, fftwOutput, FFTW_FORWARD, FFTW_MEASURE);
    std::cout << "Plan created!" << std::endl;


    // Create shared data structures
    SharedData sharedData;
    SynchronizationFlags syncFlags;

    sharedData.samplesPerBuffer = N;

    try{
        // Start the threads
        std::thread acquisitionThread(&ATS::AcquireDataMultithreadedContinuous, &alazarCard, std::ref(sharedData), std::ref(syncFlags));
        std::thread FFTThread(FFTThread, plan, N, std::ref(sharedData), std::ref(syncFlags));
        std::thread magnitudeThread(magnitudeThread, N, std::ref(sharedData), std::ref(syncFlags));
        std::thread decisionMakingThread(decisionMakingThread, std::ref(sharedData), std::ref(syncFlags));

        #if SAVE_DATA
        std::thread savingThread(saveDataToBin, std::ref(sharedData), std::ref(syncFlags));
        #endif

        // Wait for the threads to finish
        acquisitionThread.join();
        FFTThread.join();
        magnitudeThread.join();
        decisionMakingThread.join();

        #if SAVE_DATA
        savingThread.join();
        #endif
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }


    // // Main acquisition and processing logic
    // DWORD startTickCount = GetTickCount();

    // double fullTime_sec = (GetTickCount() - startTickCount) / 1000.;
    // double expectedScanTime = (samplesPerAcquisition/sampleRate)*numShots;
    // printf("\nMultithreaded run completed in %.3lf sec\n", fullTime_sec);
    // printf("Time actively scanning %.3lf sec\n", scanTime_sec);
    // printf("Expected scan time %.3lf sec\n", expectedScanTime);
    // printf("Total live time efficiency: %.3lf \n\n", expectedScanTime/scanTime_sec);


    // Cleanup
    fftw_export_wisdom_to_filename(wisdomFilePath);
    std::cout << "FFTW wisdom saved to file." << std::endl;

    fftw_destroy_plan(plan);
    fftw_free(fftwInput);
    fftw_free(fftwOutput);

    return 0;
}
