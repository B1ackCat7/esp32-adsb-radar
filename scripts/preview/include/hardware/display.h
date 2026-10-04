#pragma once
#include "Arduino.h"
#include <LovyanGFX.hpp>
// The panel is another RGB565 sprite, with no display driver attached.
using LGFX = lgfx::LGFX_Sprite;
extern LGFX tft;
void displayInit();
