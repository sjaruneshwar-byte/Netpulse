#ifndef INPUT_VALIDATOR_H
#define INPUT_VALIDATOR_H

#include "telemetry_parser.h"

bool validateTelemetry(
    const ParsedTelemetry& telemetry,
    std::string& reason
);

#endif
