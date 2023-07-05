/**
 * @file ATS.hpp
 * @author Kyle Quinlan (kyle.quinlan@colorado.edu)
 * @brief Class definition for ATS class. This class is used to control the AlazarTech ATS9462 card.
 * @version 0.1
 * @date 2023-06-27
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#ifndef ATS_H
#define ATS_H

#include "decs.hpp"

/**
 * @brief Struct for storing acquisition parameters. Set by setAcquisitionParameters() method and accessed throughout the class.
 * 
 */
struct AcquisitionParameters {
    U32 sampleRate;
    U32 samplesPerAcquisition;
    U32 buffersPerAcquisition;
    U32 recordsPerAcquisition;

    double inputRange;
    double inputImpedance;

    U32 samplesPerBuffer;
    U32 bytesPerSample;
    U32 bytesPerBuffer;
};

/**
 * @brief Struct for storing data shared between threads. Used for multithreaded data acquisition.
 * 
 */
struct SharedData {
    std::mutex mutex;

    int samplesPerBuffer;

    std::queue<fftw_complex*> dataQueue;
    std::queue<fftw_complex*> dataSavingQueue;
    std::queue<fftw_complex*> processedDataQueue;

    std::condition_variable dataReadyCondition;
    std::condition_variable saveReadyCondition;
    std::condition_variable processedDataReadyCondition;
};

/**
 * @brief Struct for storing synchronization flags. Used for multithreaded data acquisition.
 * 
 */
struct SynchronizationFlags {
    std::mutex mutex;
    bool pauseDataCollection;
    bool acquisitionComplete;
    bool dataReady;
    bool dataProcessingComplete;

    SynchronizationFlags() : pauseDataCollection(false), acquisitionComplete(false),
                             dataReady(false), dataProcessingComplete(false) {}
};


/**
 * @brief Class for controlling alazarCard. Implements methods for acquiring data and setting acquisition parameters. 
 * Function definitions and documentation are in ATS.cpp.
 * 
 */
class ATS {
public:
    ATS(int systemId = 1, int boardId = 1);
    ~ATS();

    double setExternalSampleClock(double requestedSampleRate);
    void setAcquisitionParameters(U32 sampleRate, U32 samplesPerAcquisition, U32 buffersPerAcquisition=1, double inputRange=0.8, double inputImpedance=50);
    void setInputParameters(char channel, std::string coupling, double inputRange, double inputImpedance=50);
    void toggleLowPass(char channel, bool enable);

    fftw_complex* AcquireData();
    void AcquireDataMultithreadedContinuous(SharedData& sharedData, SynchronizationFlags& syncFlags);

    U32 suggestBufferNumber(U32 sampleRate, U32 samplesPerAcquisition);

    AcquisitionParameters acquisitionParams;

private:
    HANDLE boardHandle;
    RETURN_CODE retCode;

    IO_BUFFER *IoBufferArray[BUFFER_COUNT] = { NULL };

    int getChannelID(char channel);
};


std::pair<std::vector<double>, std::vector<double>> processData(std::pair<std::vector<unsigned short>, std::vector<unsigned short>> sampleData, AcquisitionParameters acquisitionParams);

fftw_complex* processDataFFT(fftw_complex* sampleData, fftw_plan plan, int N);

#endif // ATS_H
