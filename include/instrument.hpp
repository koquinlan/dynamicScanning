#ifndef INST_H
#define INST_H

#include "decs.hpp"

class Instrument {
protected:
    ViSession instrumentSession;
    ViSession defaultRM;

    ViStatus status;

public:
    Instrument();
    virtual ~Instrument();

    void openConnection(int gpibAddress);
    void closeConnection();

    void sendCustomCommand(const std::string& command);
    std::string sendCustomQuery(std::string query);
    std::string queryError();

    virtual void onOff(bool on);
    void reset();
};

#endif // INST_H