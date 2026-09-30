#pragma once

#include <cstddef>
#include <cstdint>

namespace ui::radar {

/** Radar pages retain the established ring-3 scale-label convention. */
struct RangePreset {
  /** Distance shown on ring 3 (¾ of outer radius), always stored in km. */
  float ring3_km;
  float outer_km;
};

constexpr float kRing3ToOuterKm = 4.0f / 3.0f;

constexpr RangePreset kRangePresets[] = {
    {25.0f, 25.0f * kRing3ToOuterKm},
    {50.0f, 50.0f * kRing3ToOuterKm},
    {100.0f, 100.0f * kRing3ToOuterKm},
};

constexpr size_t kRangePresetCount =
    sizeof(kRangePresets) / sizeof(kRangePresets[0]);

/** Initialize 25 km view and load distance units from flash. Call once after boot. */
void rangeInit();
/** Select a radar page in RAM; never write flash during page changes. */
void rangeSelect(uint8_t index);
const RangePreset& rangeCurrent();
uint8_t rangeIndex();
/** ADSB fetch radius (km): scaled to screen edge so beyond-ring dots have data. */
float fetchRadiusKm();
/** Fixed shared collection radius, independent of the selected page. */
float collectionRadiusKm();

bool useMiles();
bool showRunways();
/** WiFi portal checkbox: "T" = miles, otherwise km. */
void saveMilesFromPortal(const char* checkbox_value);
void saveRunwaysFromPortal(const char* checkbox_value);
void formatRing3Label(char* buf, size_t len, float ring3_km, bool use_miles);
void formatCurrentRing3Label(char* buf, size_t len);
/** Reset distance units to km (e.g. with WiFi credential wipe). */
void unitsReset();

}  // namespace ui::radar
