#pragma once

namespace services::location {

/** Load saved lat/lon from NVS, or use config defaults. Call once before WiFi setup. */
void init();

/** Saved coordinates, or neutral internal values when unconfigured. */
double lat();
double lon();
bool configured();

/** Parse portal strings, validate, persist to NVS, update runtime values. */
bool saveFromStrings(const char* lat_str, const char* lon_str);

/** Clear stored coordinates (e.g. with WiFi credential reset). */
void clear();

}  // namespace services::location
