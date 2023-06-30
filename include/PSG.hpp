/**
 * @file PSG.hpp
 * @author Kyle Quinlan (kyle.quinlan@colorado.edu)
 * @brief Instrument class for controlling Agilent PSG generators (tested on E8257N)
 * @version 0.1
 * @date 2023-06-30
 * 
 * @copyright Copyright (c) 2023
 * 
 */

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
