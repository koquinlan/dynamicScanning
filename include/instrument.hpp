/**
 * @file instrument.hpp
 * @author Kyle Quinlan (kyle.quinlan@colorado.edu)
 * @brief Parent instrument class for GPIB/VISA instruments to inherit from
 * @version 0.1
 * @date 2023-06-30
 * 
 * @copyright Copyright (c) 2023
 * 
 */

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