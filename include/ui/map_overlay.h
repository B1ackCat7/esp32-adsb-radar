#pragma once
#include <LovyanGFX.hpp>
namespace ui::map {
void invalidateCache();
const char* coverage();
uint32_t rebuildMicros();
size_t selectedEdges();
size_t cachedSpans();
void draw(lgfx::LGFXBase& g);
}
