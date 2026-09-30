#include "ui/station_display.h"

#include <WiFi.h>

#include <cstdio>

#include "hardware/display.h"
#include "hardware/display_font.h"
#include "services/station_status.h"
#include "ui/display_theme.h"
#include "ui/radar_display.h"
namespace ui {
namespace {
constexpr auto bg = theme::background, white = theme::white,
               muted = theme::muted, line = theme::grid;
constexpr auto cyan = theme::cyan, green = theme::cyan, red = theme::danger;
void text(lgfx::LGFXBase& g, const char* value, int x, int y, int size,
          uint16_t color) {
  if (size >= 20)
    displayFontSetBitmap(g, size >= 30 ? &fonts::FreeSansBold18pt7b
                                       : &fonts::FreeSansBold12pt7b);
  else
    g.setFont(&fonts::Font2);
  g.setTextSize(1);
  g.setTextColor(color, bg);
  g.setTextDatum(textdatum_t::middle_center);
  g.drawString(value, x, y);
}
}  // namespace
void stationDisplayDraw() {
  auto& g = sharedDisplayFrame();
  const auto& s = services::station::displayStatus();
  const auto& feed = services::station::status().feed;
  const uint32_t now = millis();
  const bool live = services::station::online();
  const bool delayed =
      live && feed.valid &&
      uint32_t(now - feed.advancedAt) >= services::station::kFeedDelayedMs;
  const auto stateColor = !live ? red : delayed ? theme::orange : cyan;
  g.fillScreen(bg);
  g.drawCircle(120, 120, 115, line);
  g.drawFastHLine(100, 14, 40, theme::orange);
  text(g, "HOME STATION", 120, 30, 14, muted);
  g.fillCircle(76, 53, 3, stateColor);
  text(g,
       !live     ? "OFFLINE"
       : delayed ? "DELAYED"
                 : "ONLINE",
       124, 53, 14, stateColor);
  char value[32];
  if (live && s.rateValid && uint32_t(now - s.rateAt) < 30000)
    snprintf(value, sizeof(value), "%.0f", s.messagesPerSecond);
  else
    snprintf(value, sizeof(value), "--");
  text(g, value, 120, 90, 36, theme::orange);
  text(g, "MESSAGES / SEC", 120, 119, 14, muted);
  g.drawFastHLine(43, 138, 154, line);
  g.drawFastVLine(120, 150, 40, line);
  if (live && !delayed && s.feed.valid)
    snprintf(value, sizeof(value), "%u", s.tracked);
  else
    snprintf(value, sizeof(value), "--");
  text(g, value, 77, 158, 24, cyan);
  text(g, "TRACKED", 77, 184, 14, muted);
  const bool temperatureFresh =
      live && s.temperatureValid && uint32_t(now - s.temperatureAt) < 30000;
  if (temperatureFresh)
    snprintf(value, sizeof(value), "%.0f C", s.temperatureC);
  else
    snprintf(value, sizeof(value), "--");
  text(g, value, 166, 158, 24,
       theme::temperatureColor(s.temperatureC, temperatureFresh));
  text(g, "CPU TEMP", 166, 184, 14, muted);
  if (WiFi.status() != WL_CONNECTED)
    snprintf(value, sizeof(value), "Wi-Fi disconnected");
  else if (!feed.valid)
    snprintf(value, sizeof(value), "Waiting for receiver");
  else if (!live)
    snprintf(value, sizeof(value), "No fresh receiver data");
  else if (delayed)
    snprintf(value, sizeof(value), "Receiver data delayed");
  else if (!s.rateValid)
    snprintf(value, sizeof(value), "Collecting station stats");
  else if (uint32_t(now - s.rateAt) >= 20000)
    snprintf(value, sizeof(value), "Stats update delayed");
  else
    snprintf(value, sizeof(value), "Refresh every 10s");
  text(g, value, 120, 208, 14, muted);
  for (int i = 0; i < 4; ++i)
    g.fillCircle(101 + i * 12, 229, i == 3 ? 3 : 2,
                 i == 3 ? theme::orange : line);
  pushSharedDisplayFrame();
}
void displayScreenshot() {
  auto& g = sharedDisplayFrame();
  Serial.print("\nP6\n240 240\n255\n");
  uint8_t row[240 * 3];
  for (int y = 0; y < 240; ++y) {
    for (int x = 0; x < 240; ++x) {
      uint16_t c = g.readPixel(x, y);
      row[3 * x] = ((c >> 11) & 31) * 255 / 31;
      row[3 * x + 1] = ((c >> 5) & 63) * 255 / 63;
      row[3 * x + 2] = (c & 31) * 255 / 31;
    }
    Serial.write(row, sizeof(row));
  }
  Serial.flush();
}
}  // namespace ui
