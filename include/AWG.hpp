#ifndef AWG_H
#define AWG_H

#include <string>
#include <vector>

#include <visa.h>

#include "instrument.hpp"

class AWG : public Instrument {
public:
    AWG(int gpibAddress);
    ~AWG();

    void onOff(bool on) override;
    void onOff(bool on, int channel);

    void setSampleRate(double rate, int channel=1);
    void setAmp(double amp, int channel=1);
    void setOffset(double offset, int channel=1);

    void AWG::clearMemory(int channel=1);
    void sendWaveform(std::vector<double> waveform, std::string name, int channel=1);
};

#endif // AWG_H
