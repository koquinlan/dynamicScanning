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
#define DETECT_BAD_BINS (0)
#define REFRESH_BASELINE (0)

int main() {
    // std::chrono::seconds dura(5);
    // std::this_thread::sleep_for(dura);


    // Set up acquisition parameters
    double sampleRate = 32e6;
    double RBW = 100;
    int maxSpectraPerAcquisition = 250;

    double samplesPerSpectrum = sampleRate/RBW;
    double samplesPerAcquisition = samplesPerSpectrum*maxSpectraPerAcquisition;


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
    alazarCard.setAcquisitionParameters((U32)sampleRate, (U32)samplesPerAcquisition, maxSpectraPerAcquisition);
    std::cout << "Acquisition parameters set. Collecting " << std::to_string(alazarCard.acquisitionParams.buffersPerAcquisition) << " buffers." << std::endl;

    int N = (int)alazarCard.acquisitionParams.samplesPerBuffer;
    std::cout << "Creating plan for N = " << std::to_string(N) << std::endl;
    fftw_complex* fftwInput = reinterpret_cast<fftw_complex*>(fftw_malloc(sizeof(fftw_complex) * N));
    fftw_complex* fftwOutput = reinterpret_cast<fftw_complex*>(fftw_malloc(sizeof(fftw_complex) * N));
    fftw_plan plan = fftw_plan_dft_1d(N, fftwInput, fftwOutput, FFTW_FORWARD, FFTW_MEASURE);
    std::cout << "Plan created!" << std::endl;


    // Create data processor
    DataProcessor dataProcessor;
    double cutoffFrequency = 10e3;
    int poleNumber = 3;
    double stopbandAttenuation = 15.0;
    dataProcessor.setFilterParams(alazarCard.acquisitionParams.sampleRate, poleNumber, cutoffFrequency, stopbandAttenuation);

    // Try to import bad bins if available
    #if !DETECT_BAD_BINS
    std::vector<double> badBins = readVector("badBins.csv");
    dataProcessor.badBins.reserve(badBins.size());

    std::transform(badBins.begin(), badBins.end(), std::back_inserter(dataProcessor.badBins), [](double d) { return static_cast<int>(d); });
    #endif

    #if !REFRESH_BASELINE
    dataProcessor.currentBaseline = readVector("baseline.csv");
    #endif


    // Create shared data structures
    SharedData sharedData;
    SynchronizationFlags syncFlags;

    sharedData.samplesPerBuffer = N;

    try{
        // Start the threads
        std::thread acquisitionThread(&ATS::AcquireDataMultithreadedContinuous, &alazarCard, std::ref(sharedData), std::ref(syncFlags));
        std::thread FFTThread(FFTThread, plan, N, std::ref(sharedData), std::ref(syncFlags));
        std::thread magnitudeThread(magnitudeThread, N, std::ref(sharedData), std::ref(syncFlags), std::ref(dataProcessor));
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

    dataProcessor.updateBaseline();
    std::vector<double> processedData, processedBaseline;
    std::tie(processedData, processedBaseline) = dataProcessor.rawToProcessed(dataProcessor.runningAverage);


    // Cleanup
    fftw_export_wisdom_to_filename(wisdomFilePath);
    std::cout << "FFTW wisdom saved to file." << std::endl;

    fftw_destroy_plan(plan);
    fftw_free(fftwInput);
    fftw_free(fftwOutput);

    reportPerformance();


    // Save the data
    std::vector<double> freq(N);
    for (int i = 0; i < N; ++i) {
        freq[i] = (static_cast<double>(i)-static_cast<double>(N)/2)*alazarCard.acquisitionParams.sampleRate/N/1e6;
    }

    std::vector<int> outliers = findOutliers(dataProcessor.runningAverage, 50, 5);

    saveVector(freq, "../../../plotting/freq.csv");
    saveVector(outliers, "../../../plotting/outliers.csv");

    saveVector(dataProcessor.currentBaseline, "../../../plotting/baseline.csv");
    saveVector(dataProcessor.runningAverage, "../../../plotting/runningAverage.csv");

    saveVector(processedData, "../../../plotting/processedData.csv");
    saveVector(processedBaseline, "../../../plotting/processedBaseline.csv");

    #if DETECT_BAD_BINS
    saveVector(outliers, "badBins.csv");
    #endif

    return 0;
}
