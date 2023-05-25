#ifndef PSG_H
#define PSG_H

#include <visa.h>

class PSG {
public:
    PSG(int gpibAddress);
    ~PSG();

    void onOff(bool on);
    void setFreq(double frequency);

private:
    ViSession vi;
};

#endif // PSG_H
