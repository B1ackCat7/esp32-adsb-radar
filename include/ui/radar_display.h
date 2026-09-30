#pragma once
#include <LovyanGFX.hpp>

namespace ui {
lgfx::LovyanGFX& sharedDisplayFrame();
void pushSharedDisplayFrame();

/** Draw the static sonar/radar grid (black disc, green overlay, labels). */
void radarDisplayDraw();

/** Redraw aircraft only (blits cached grid; no full-screen clear). */
void radarDisplayRefreshAircraft();

}  // namespace ui
