/**
 * @file controlTests.cpp
 * @author Kyle Quinlan (kyle.quinlan@colorado.edu)
 * @brief For rapid prototyping and testing of single threaded control and acquisition code. See src/threadedTesting.cpp for multithreaded development.
 * @version 0.1
 * @date 2023-06-30
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include "decs.hpp"

int main() {
    printAvailableResources();

    // psgTesting(21);
    // awgTesting(10);

    // Try to import an FFTW plan if available
    const char* wisdomFilePath = "fftw_wisdom.txt";
    if (fftw_import_wisdom_from_filename(wisdomFilePath) != 0) {
        std::cout << "Successfully imported FFTW wisdom from file." << std::endl;
    }
    else {
        std::cout << "Failed to import FFTW wisdom from file." << std::endl;
    }

    ATS alazarCard(1, 1);
    alazarCard.setAcquisitionParameters((U32)32e6, (U32)16e4, 0);

    
    // Create an FFTW plan
    int N = (int)alazarCard.acquisitionParams.samplesPerAcquisition;

    std::cout << "Creating plan for N = " << std::to_string(N) << std::endl;
    fftw_complex* fftwInput = reinterpret_cast<fftw_complex*>(fftw_malloc(sizeof(fftw_complex) * N));
    fftw_complex* fftwOutput = reinterpret_cast<fftw_complex*>(fftw_malloc(sizeof(fftw_complex) * N));
    fftw_plan plan = fftw_plan_dft_1d(N, fftwInput, fftwOutput, FFTW_FORWARD, FFTW_ESTIMATE);
    std::cout << "Plan created!" << std::endl;


    // Acquire data
    int numSubspectra = 32;
    std::vector<fftw_complex*> rawData(numSubspectra);
    std::vector<fftw_complex*> procData(numSubspectra);

    for (int i = 0; i < numSubspectra; i++) {
        rawData[i] = alazarCard.AcquireData();
        procData[i] = processDataFFT(rawData[i], plan, N);
    }


    // Process the data into voltages
    std::vector<double> freq(N);
    std::vector<std::vector<double>> fftVoltage(numSubspectra);
    std::vector<std::vector<double>> fftPower(numSubspectra);

    for (int i = 0; i < N; ++i) {
        freq[i] = (static_cast<double>(i)-static_cast<double>(N)/2)*alazarCard.acquisitionParams.sampleRate/N/1e6;
    }

    for (int i=0; i < numSubspectra; i++) {
        for (int j=0; j < N; j++){
            fftVoltage[i].push_back( std::sqrt((procData[i][j][0]*procData[i][j][0] + procData[i][j][1]*procData[i][j][1]))/(double)N );

            fftPower[i].push_back( fftVoltage[i][j]*fftVoltage[i][j]/(double)alazarCard.acquisitionParams.inputImpedance );
        }
    }


    // Get the average
    std::vector<double> fftVoltageAvg(N);
    std::vector<double> fftPowerAvg(N);

    for (int i=0; i < N; i++) {
        for (int j=0; j < numSubspectra; j++) {
            fftVoltageAvg[i] += fftVoltage[j][i];
            fftPowerAvg[i] += fftPower[j][i];
        }
        fftVoltageAvg[i] /= numSubspectra;
        fftPowerAvg[i] /= numSubspectra;
    }


    // Plot the data
    plt::figure();
    
    // plt::plot(freq, fftVoltage[0]);
    // plt::plot(freq, fftVoltageAvg);

    plt::plot(freq, fftPower[0]);
    plt::plot(freq, fftPowerAvg);
    plt::xlim(-10, 10);
    plt::ylim(0., 8e-9);
    
    plt::xlabel("Frequency (MHz)");
    plt::show();


    // Save the plan using FFTW wisdom
    fftw_export_wisdom_to_filename(wisdomFilePath);
    std::cout << "FFTW wisdom saved to file." << std::endl;


    // Cleanup
    fftw_destroy_plan(plan);
    fftw_free(fftwInput);
    fftw_free(fftwOutput);

    return 0;
}
