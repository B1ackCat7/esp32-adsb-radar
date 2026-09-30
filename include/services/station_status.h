#pragma once
#include <cstdint>

#include "services/station_logic.h"
namespace services::station {
struct Status {
  Freshness feed;
  unsigned tracked = 0;
  unsigned plotted = 0;
  float messagesPerSecond = 0;
  bool rateValid = false;
  uint32_t rateAt = 0;
  float temperatureC = 0;
  bool temperatureValid = false;
  uint32_t temperatureAt = 0;
};
const Status& status();
const Status& displayStatus();
void commitDisplay();
bool online();
void expire();
void fetchMetrics(bool temperature);
}  // namespace services::station
