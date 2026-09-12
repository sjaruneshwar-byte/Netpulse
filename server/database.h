#ifndef DATABASE_H
#define DATABASE_H

#include <string>

#include "telemetry_parser.h"
#include "fault_detector.h"

class Database
{
public:

    Database();

    ~Database();

    bool initialize(
        const std::string& databasePath
    );

    bool saveTelemetry(
        const ParsedTelemetry& telemetry
    );

    bool saveFault(
        const FaultEvent& fault
    );

private:

    void* db;
};

#endif