#pragma once
#include <Arduino.h>

namespace sixback {
    void startMicrolink();
    bool handleMicrolinkSerialCommand(const String& line);
}
