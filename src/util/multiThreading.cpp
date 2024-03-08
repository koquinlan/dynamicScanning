#include "decs.hpp"

void fftThread(fftw_plan plan, int samplesPerSpectrum, ThreadSafeQueue<fftw_complex*>& inputQueue, ThreadSafeQueue<fftw_complex*>& outputQueue){
    while (true) {
        std::shared_ptr<fftw_complex*> rawDataPointer = inputQueue.waitAndPop();

        startTimer(TIMER_FFT);
        fftw_complex* rawData = *rawDataPointer;
        fftw_complex* fftData = processDataFFT(rawData, plan, samplesPerSpectrum);
    
        fftw_free(rawData);
        stopTimer(TIMER_FFT);

        // The inputComplete flag should be thrown while pushing the last data to the output queue, before the condition variable is notified
        if (inputQueue.isInputComplete() && inputQueue.empty()) {
            outputQueue.pushFinal(fftData);
            break;
        }
        else {
            outputQueue.push(fftData);
        }
    }   
}


void magnitudeThread(int samplesPerSpectrum, DataProcessor& dataProcessor, ThreadSafeQueue<fftw_complex*>& inputQueue, ThreadSafeQueue<std::vector<double>>& outputQueue){
    while (true) {
        std::shared_ptr<fftw_complex*> fftDataPointer = inputQueue.waitAndPop();

        startTimer(TIMER_MAG);
        fftw_complex* fftData = *fftDataPointer;

        // Main processing logic
        std::vector<double> magData(samplesPerSpectrum);
        for (int i = 0; i < samplesPerSpectrum; i++) {
            magData[i] = ( fftData[i][0]*fftData[i][0] + fftData[i][1]*fftData[i][1] ) / samplesPerSpectrum / 50; // Hard code in 50 Ohm input impedance
        }
        magData = dataProcessor.trimDC(dataProcessor.removeBadBins(magData));
        fftw_free(fftData);

        stopTimer(TIMER_MAG);

        if (inputQueue.isInputComplete() && inputQueue.empty()) {
            outputQueue.pushFinal(magData);
            break;
        }
        else {
            outputQueue.push(magData);
        }
    }
}


void averagingThread(DataProcessor& dataProcessor, double trueCenterFreq, 
                    ThreadSafeQueue<std::vector<double>>& inputQueue, ThreadSafeQueue<Spectrum>& outputQueue, 
                    int subSpectraAveragingNumber = 20) 
{
    std::vector<std::vector<double>> subSpectra;
    int subSpectraAveraged = 0;

    while (true) {
        std::shared_ptr<std::vector<double>> magDataPointer = inputQueue.waitAndPop();

        startTimer(TIMER_AVERAGE);
        std::vector<double> magData = *magDataPointer;

        // Immediately unpack the data into a growing vector of subSpectra
        subSpectra.push_back(magData);
        dataProcessor.addRawSpectrumToRunningAverage(magData);

        // If the subSpectra vector is ready, average it and push it to the output queue
        if (subSpectra.size() == subSpectraAveragingNumber || (inputQueue.isInputComplete() && inputQueue.empty())) {
            Spectrum rawSpectrum;
            rawSpectrum.powers = averageVectors(subSpectra);
            rawSpectrum.freqAxis = dataProcessor.SNR.freqAxis;
            rawSpectrum.trueCenterFreq = trueCenterFreq;

            subSpectraAveraged += (int)subSpectra.size();

            subSpectra.clear();

            if (inputQueue.isInputComplete() && inputQueue.empty()) {
                stopTimer(TIMER_AVERAGE);
                outputQueue.pushFinal(rawSpectrum);

                setMetric(ACQUIRED_SPECTRA, subSpectraAveraged);
                setMetric(SPECTRUM_AVERAGE_SIZE, subSpectraAveragingNumber);
                break;
            }
            else {
                outputQueue.push(rawSpectrum);
            }
        }
        stopTimer(TIMER_AVERAGE);
    }
}


void processingThread(DataProcessor& dataProcessor, ThreadSafeQueue<Spectrum>& inputQueue, ThreadSafeQueue<CombinedSpectrum>& outputQueue) {
    while (true) {
        std::shared_ptr<Spectrum> rawSpectrumPointer = inputQueue.waitAndPop();

        startTimer(TIMER_PROCESS);
        Spectrum rawSpectrum = *rawSpectrumPointer;

        // Main processing logic
        Spectrum processedSpectrum, foo;
        std::tie(processedSpectrum, foo) = dataProcessor.rawToProcessed(rawSpectrum);

        trimSpectrum(processedSpectrum, 0.1);
        dataProcessor.trimSNRtoMatch(processedSpectrum);

        Spectrum rescaledSpectrum = dataProcessor.processedToRescaled(processedSpectrum);

        CombinedSpectrum combinedSpectrum;
        dataProcessor.addRescaledToCombined(rescaledSpectrum, combinedSpectrum);

        CombinedSpectrum rebinnedSpectrum = dataProcessor.rebinCombinedSpectrum(combinedSpectrum, 10, 1);

        stopTimer(TIMER_PROCESS);

        if (inputQueue.isInputComplete() && inputQueue.empty()) {
            outputQueue.pushFinal(rebinnedSpectrum);
            break;
        }
        else {
            outputQueue.push(rebinnedSpectrum);
        }
    }
}


void decisionMakingThread(BayesFactors& bayesFactors, DecisionAgent& decisionAgent, ThreadSafeQueue<CombinedSpectrum>& inputQueue, std::atomic<bool>& triggerEnd) {
    setMetric(SPECTRA_AT_DECISION, -1);
    
    int buffersDecided = 0;

    while (true) {
        std::shared_ptr<CombinedSpectrum> rebinnedSpectrumPointer = inputQueue.waitAndPop();

        startTimer(TIMER_DECISION);
        CombinedSpectrum rebinnedSpectrum = *rebinnedSpectrumPointer;

        if (decisionAgent.trimmedSNR.powers.empty()) {
            decisionAgent.resizeSNRtoMatch(rebinnedSpectrum);
            decisionAgent.setTargets();
            decisionAgent.setPoints();
        }

        bayesFactors.updateExclusionLine(rebinnedSpectrum);

        if (!triggerEnd.load()){
            std::vector<double> activeWindow(bayesFactors.exclusionLine.powers.end() - decisionAgent.trimmedSNR.powers.size(), bayesFactors.exclusionLine.powers.end());
            int decision = decisionAgent.getDecision(activeWindow, buffersDecided);

            buffersDecided++;

            if (decision) {
                triggerEnd = true;
            }
        }
        stopTimer(TIMER_DECISION);


        if (inputQueue.isInputComplete() && inputQueue.empty()) {
            updateMetric(SPECTRA_AT_DECISION, buffersDecided);
            break;
        }
    }
}