#ifndef PSG_H
#define PSG_H

#include <visa.h>

class PSG {
public:
    PSG(const char* deviceAddress);
    ~PSG();

    void onOff(bool on);
    void setFreq(double frequency);

private:
    ViSession vi;
};

#endif // PSG_H
