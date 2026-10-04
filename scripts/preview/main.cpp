#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

#include "hardware/display.h"
#include "hardware/display_font.h"
#include "services/adsb_client.h"
#include "services/radar_location.h"
#include "services/station_status.h"
#include "ui/display_theme.h"
#include "ui/radar_display.h"
#include "ui/radar_range.h"
#include "ui/station_display.h"

LGFX tft;
namespace {
// Public city-center example, unrelated to an installed receiver/home location.
constexpr double centerLat = -33.87, centerLon = 151.21;  // Sydney
services::adsb::Aircraft aircraft[8]{};
services::station::Status demoStatus;

void writeFrame(lgfx::LGFXBase& frame, const std::string& path) {
  FILE* file = std::fopen(path.c_str(), "wb");
  assert(file);
  std::fprintf(file, "P6\n%d %d\n255\n", frame.width(), frame.height());
  for (int y = 0; y < frame.height(); ++y)
    for (int x = 0; x < frame.width(); ++x) {
      const uint16_t c = frame.readPixel(x, y);
      const uint8_t pixel[] = {uint8_t(((c >> 11) & 31) * 255 / 31),
                               uint8_t(((c >> 5) & 63) * 255 / 63),
                               uint8_t((c & 31) * 255 / 31)};
      std::fwrite(pixel, 1, 3, file);
    }
  std::fclose(file);
}

void populate() {
  struct Example { float east, north, heading; const char* type; int altitude; };
  const Example examples[] = {
      {-15, 15, 140, "A320", 12000}, {20, 0, 45, "B738", 8000},
      {-29, -28, 315, "C172", 3500}, {42, 32, 200, "A359", 22000},
      {-45, -40, 80, "B789", 18000}, {80, 60, 255, "A332", 31000},
      {-95, 30, 120, "B77W", 34000}, {25, -100, 15, "A321", 26000}};
  for (size_t i = 0; i < 8; ++i) {
    auto& ac = aircraft[i];
    ac.lat = centerLat + examples[i].north / 111.f;
    ac.lon = centerLon + examples[i].east /
                            (111.f * std::cos(centerLat * 3.14159265 / 180));
    ac.observedAt = millis();
    ac.positionAge = 1;
    ac.nose_deg = ac.track_deg = examples[i].heading;
    ac.gs_knots = i == 2 ? 110 : 260;
    std::snprintf(ac.hex, sizeof(ac.hex), "%06x", unsigned(i + 1));
    std::snprintf(ac.callsign, sizeof(ac.callsign), "DEMO%03u", unsigned(i + 101));
    std::snprintf(ac.type, sizeof(ac.type), "%s", examples[i].type);
    std::snprintf(ac.alt, sizeof(ac.alt), "%d ft", examples[i].altitude);
  }
  demoStatus.feed.accept(1000, millis());
  demoStatus.tracked = 14;  // Includes synthetic targets without usable positions.
  demoStatus.plotted = 8;
  demoStatus.messagesPerSecond = 872;
  demoStatus.rateValid = demoStatus.temperatureValid = true;
  demoStatus.rateAt = demoStatus.temperatureAt = millis();
  demoStatus.temperatureC = 47;
}
}

namespace services::location {
double lat() { return centerLat; }
double lon() { return centerLon; }
bool configured() { return true; }
}
namespace services::adsb {
size_t aircraftCount() { return 8; }
const Aircraft* aircraftList() { return aircraft; }
}
namespace services::station {
const Status& status() { return demoStatus; }
const Status& displayStatus() { return demoStatus; }
bool online() { return demoStatus.feed.online(millis()); }
}

int main(int argc, char** argv) {
  assert(argc == 2);
  const std::string out = argv[1];
  tft.setColorDepth(16);
  assert(tft.createSprite(240, 240));
  assert(displayFontInit());
  ui::radar::rangeInit();
  populate();
  const char* names[] = {"radar-25km", "radar-50km", "radar-100km"};
  for (uint8_t range = 0; range < 3; ++range) {
    ui::radar::rangeSelect(range);
    ui::radarDisplayDraw();
    writeFrame(ui::sharedDisplayFrame(), out + "/" + names[range] + ".ppm");
  }
  ui::stationDisplayDraw();
  writeFrame(ui::sharedDisplayFrame(), out + "/station.ppm");

  // The overview's headings/captions also use the library's bundled font.
  LGFX overview;
  overview.setColorDepth(16);
  assert(overview.createSprite(560, 690));
  overview.fillScreen(ui::theme::background);
  overview.setTextDatum(textdatum_t::middle_center);
  overview.setFont(&fonts::FreeSansBold12pt7b);
  overview.setTextColor(ui::theme::white, ui::theme::background);
  overview.drawString("PLANE RADAR", 280, 28);
  overview.setFont(&fonts::Font2);
  overview.setTextColor(ui::theme::muted, ui::theme::background);
  overview.drawString("Software-rendered previews / synthetic data", 280, 54);
  const char* titles[] = {"25 km / aircraft labels", "50 km / symbols", "100 km / symbols", "Station / receiver health"};
  for (int i = 0; i < 4; ++i) {
    const int x = 20 + (i % 2) * 280, y = 92 + (i / 2) * 280;
    overview.setTextColor(ui::theme::cyan, ui::theme::background);
    overview.drawString(titles[i], x + 120, y - 12);
    if (i < 3) {
      ui::radar::rangeSelect(i);
      ui::radarDisplayDraw();
    } else ui::stationDisplayDraw();
    auto& frame = ui::sharedDisplayFrame();
    // Mask only the pixels outside the round physical viewport.
    for (int py = 0; py < 240; ++py)
      for (int px = 0; px < 240; ++px)
        if ((px - 120) * (px - 120) + (py - 120) * (py - 120) <= 120 * 120)
          overview.drawPixel(x + px, y + py, uint16_t(frame.readPixel(px, py)));
  }
  overview.setTextColor(ui::theme::muted, ui::theme::background);
  overview.drawString("Sydney example / offline Natural Earth map", 280, 646);
  overview.drawString("Logical RGB colors / not photographs of a panel", 280, 670);
  writeFrame(overview, out + "/overview.ppm");
}
