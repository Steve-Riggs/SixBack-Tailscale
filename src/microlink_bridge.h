#pragma once
#include <Arduino.h>

namespace sixback {
    void startMicrolink();
    void microlinkTick();
    bool handleMicrolinkSerialCommand(const String& line, Stream& output);
}
