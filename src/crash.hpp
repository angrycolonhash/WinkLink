#ifndef CRASH_HPP
#define CRASH_HPP

#include "Arduino.h"
#include "TFT_eSPI.h"
#include "drawing.hpp"

// Only declare the function here, implementation will be in a .cpp file
void fatal_crash(TFT_eSPI &tft, String reason);

#endif