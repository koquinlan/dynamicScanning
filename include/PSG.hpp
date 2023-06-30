#ifndef PSG_H
#define PSG_H

#include "decs.hpp"

class PSG : public Instrument{
public:
    PSG(int gpibAddress);
    ~PSG();

    void setFreq(double frequency);
    void setPow(double pow);

    void modOnOff(bool on);
    void setFreqModDev(double dev);
    void freqModOnOff(bool on);
    void setFreqModSrc(bool ext, int src);
};

#endif // PSG_H
