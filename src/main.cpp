/** Local receiver extension of MatixYo Plane Radar. */
#include <Arduino.h>
#include <WiFi.h>

#include "config.h"
#include "hardware/display.h"
#include "services/adsb_client.h"
#include "services/poll_schedule.h"
#include "services/radar_location.h"
#include "services/station_settings.h"
#include "services/station_status.h"
#include "services/wifi_setup.h"
#include "ui/map_overlay.h"
#include "ui/page_cycle.h"
#include "ui/radar_display.h"
#include "ui/radar_range.h"
#include "ui/station_display.h"
#include "ui/status_screens.h"
namespace {
bool statsPage = false, redraw = true;
unsigned long lastReconnect = 0, wifiDownAt = 0;
services::PollSchedule polls;
services::DisplayCycle stationCycle;
uint32_t lastDrawUs = 0, maxDrawUs = 0, lastNetworkMs = 0, frameCount = 0;
services::station::Clicks clicks;
ui::PageCycle pageCycle;
ui::Pages pages;
void draw() {
  const uint32_t began = micros();
  services::station::expire();
  if (statsPage)
    ui::stationDisplayDraw();
  else
    ui::radarDisplayDraw();
  redraw = false;
  lastDrawUs = micros() - began;
  if (lastDrawUs > maxDrawUs) maxDrawUs = lastDrawUs;
  ++frameCount;
}
void applyPage() {
  statsPage = pages.current == ui::Pages::kStation;
  ui::radar::rangeSelect(pages.lastRadar);
  redraw = true;
  Serial.printf("page: %u scale_km=%.0f at_ms=%lu\n", pages.current,
                ui::radar::rangeCurrent().ring3_km, millis());
}
void action(int kind) {
  if (!kind) return;
  if (kind == 1) pages.next();
  if (kind == 2) pages.nextRadar();
  pageCycle.restart(millis());
  applyPage();
}
bool pageCommand(int command) {
  return command == 'r' || command == 't' || command == 'n' ||
         command == 'm' || (command >= '1' && command <= '3');
}
void handlePageCommand(int command) {
  if (command == 'n' || command == 'm') {
    action(command == 'n' ? 1 : 2);
    return;
  }
  pages.select(command == 't' ? ui::Pages::kStation
               : command == 'r' ? pages.lastRadar : command - '1');
  pageCycle.restart(millis());
  applyPage();
}
void buttons() {
  bootButtonPollLongPress();
  unsigned long at;
  while (bootButtonConsumeRelease(&at)) action(clicks.release(at));
  action(clicks.poll(millis()));
}
void refreshHealth() {
  static size_t previousCount = 0;
  services::station::expire();
  const size_t count = services::adsb::aircraftCount();
  if (!statsPage && count != previousCount) redraw = true;
  previousCount = count;
  static size_t previousHeld = 0;
  size_t held = 0;
  for (size_t i = 0; i < count; ++i)
    if (services::adsb::heldPosition(services::adsb::aircraftList()[i],
                                     millis()))
      ++held;
  if (!statsPage && held != previousHeld) redraw = true;
  previousHeld = held;
  static int previousHealth = -1;
  const auto& feed = services::station::status().feed;
  const int health = !services::station::online() ? 0
                     : uint32_t(millis() - feed.advancedAt) >=
                             services::station::kFeedDelayedMs
                         ? 1
                         : 2;
  if (health != previousHealth) redraw = true;
  previousHealth = health;
}
void cooperative() {
  wifiLoop();
  buttons();
  static uint32_t healthAt = 0;
  if (uint32_t(millis() - healthAt) >= 50) {
    healthAt = millis();
    refreshHealth();
  }
  if (stationCycle.poll(millis())) {
    services::station::commitDisplay();
    if (statsPage) redraw = true;
    Serial.printf("station cycle_ms=%lu stats_at=%lu\n", millis(),
                  (unsigned long)services::station::displayStatus().rateAt);
  }
  // Page-only serial commands are safe during HTTP body reads too.
  const int command = Serial.peek();
  if (pageCommand(command)) {
    Serial.read();
    handlePageCommand(command);
  }
  // Aircraft arrays are only committed after parsing; drawing here safely uses
  // the previous complete snapshot during a slow response-body read.
  if (!clicks.pending && !wifiBootButtonPressed() && pageCycle.poll(millis())) {
    pages.next();
    applyPage();
  }
  if (redraw) draw();
}
}  // namespace
void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.printf("Plane Radar Local %s\n", config::kFirmwareVersion);
  bootButtonInit();
  displayInit();
  services::location::init();
  ui::radar::rangeInit();
  services::station::settingsInit();
  if (wifiShowsSetupScreenOnBoot()) statusScreenPortal();
  services::adsb::setPollFn(cooperative);
  wifiSetupConnect();
  pages.select(0);
  applyPage();
  pageCycle.restart(millis());
  polls.reset(millis());
  stationCycle.reset(millis());
  services::station::commitDisplay();
  draw();
}
void loop() {
  cooperative();
  // USB diagnostics: r=radar, t=station, s=PPM framebuffer (no configuration
  // writes).
  while (Serial.available()) {
    char c = Serial.read();
    if (pageCommand(c)) {
      handlePageCommand(c);
      draw();
    }
    if (c == 's') {
      draw();
      ui::displayScreenshot();
    }
    if (c == 'b') {
      const bool savedPage = statsPage;
      statsPage = false;
      auto checksum = []() {
        auto& g = ui::sharedDisplayFrame();
        uint32_t hash = 2166136261u;
        for (int y = 0; y < 240; ++y)
          for (int x = 0; x < 240; ++x)
            hash = (hash ^ g.readPixel(x, y)) * 16777619u;
        return hash;
      };
      ui::map::invalidateCache();
      draw();
      const uint32_t cold = lastDrawUs, coldHash = checksum();
      draw();
      const uint32_t warm = lastDrawUs, warmHash = checksum();
      Serial.printf("benchmark cold_us=%u warm_us=%u identical=%d heap=%u\n",
                    cold, warm, coldHash == warmHash, ESP.getFreeHeap());
      Serial.printf("map rebuild_us=%u edges=%u spans=%u coverage=%s\n",
                    ui::map::rebuildMicros(), unsigned(ui::map::selectedEdges()),
                    unsigned(ui::map::cachedSpans()), ui::map::coverage());
      statsPage = savedPage;
      draw();
    }
    if (c == 'd') {
      Serial.printf("perf draw_us=%u max_draw_us=%u network_ms=%u frames=%u\n",
                    lastDrawUs, maxDrawUs, lastNetworkMs, frameCount);
      const auto& st = services::station::status();
      Serial.printf(
          "status online=%d tracked=%u plotted=%u msg_s=%.1f rate_valid=%d "
          "temp_c=%.1f temp_valid=%d heap=%u page=%s\n",
          services::station::online(), st.tracked, st.plotted,
          st.messagesPerSecond, st.rateValid, st.temperatureC,
          st.temperatureValid, ESP.getFreeHeap(),
          statsPage ? "station" : "radar");
    }
  }
  if (WiFi.status() == WL_CONNECTED) {
    wifiDownAt = 0;
    const auto& feed = services::station::status().feed;
    const bool metricsAllowed = feed.valid && feed.reachable &&
                                uint32_t(millis() - feed.advancedAt) <
                                    services::station::kFeedDelayedMs;
    const auto task = polls.next(millis(), metricsAllowed);
    if (!clicks.pending && task != services::PollSchedule::None) {
      const uint32_t began = millis();
      polls.begin(task, began);
      if (task == services::PollSchedule::Aircraft) {
        const bool fresh = services::adsb::fetchUpdate(
            services::location::lat(), services::location::lon(),
            ui::radar::collectionRadiusKm());
        if (fresh && !statsPage) redraw = true;
      } else {
        services::station::fetchMetrics(task ==
                                        services::PollSchedule::Temperature);
      }
      lastNetworkMs = millis() - began;
      polls.complete(millis());
    }
  } else {
    if (!wifiDownAt) wifiDownAt = millis();
    if (uint32_t(millis() - wifiDownAt) >= config::kWifiDownGraceMs &&
        uint32_t(millis() - lastReconnect) >=
            config::kWifiReconnectIntervalMs) {
      lastReconnect = millis();
      wifiReconnect();
      redraw = true;
    }
  }
  refreshHealth();
  if (redraw) draw();
  delay(10);
}
