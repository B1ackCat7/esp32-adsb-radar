#include "services/settings_portal.h"

#include <ArduinoJson.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <esp_system.h>

#include "config.h"
#include "services/adsb_client.h"
#include "services/radar_location.h"
#include "services/settings_validation.h"
#include "services/station_settings.h"
#include "services/station_status.h"
#include "ui/map_overlay.h"
#include "ui/radar_range.h"
#include "ui/settings_page.h"

namespace services::settings {
namespace {
WiFiManager* s_wm = nullptr;
String s_csrf;
uint32_t s_revision = 1;
WebServer& server() { return *s_wm->server; }
void headers() {
  server().sendHeader("Cache-Control", "no-store");
  server().sendHeader("X-Frame-Options", "DENY");
  server().sendHeader("X-Content-Type-Options", "nosniff");
}
ui::settingsPage::Values values(bool submitted = false) {
  ui::settingsPage::Values v;
  char lat[24], lon[24];
  snprintf(lat, sizeof(lat), "%.6f", location::lat());
  snprintf(lon, sizeof(lon), "%.6f", location::lon());
  v.host = submitted ? server().arg("station_host").c_str() : station::host().c_str();
  v.dataPort = submitted ? server().arg("station_data_port").c_str() : String(station::dataPort()).c_str();
  v.webPort = submitted ? server().arg("station_web_port").c_str() : String(station::webPort()).c_str();
  v.lat = submitted ? server().arg("radar_lat").c_str() : location::configured() ? lat : "";
  v.lon = submitted ? server().arg("radar_lon").c_str() : location::configured() ? lon : "";
  v.miles = submitted ? server().hasArg("use_miles") : ui::radar::useMiles();
  v.runways = submitted ? server().hasArg("show_runways") : ui::radar::showRunways();
  v.csrf = s_csrf.c_str();
  v.revision = String(s_revision).c_str();
  v.firmware = config::kFirmwareVersion;
  v.ip = (WiFi.status() == WL_CONNECTED ? WiFi.localIP() : WiFi.softAPIP()).toString().c_str();
  return v;
}
void page(int code = 200, const char* message = "", bool submitted = false) {
  auto v = values(submitted);
  v.message = message;
  v.error = code >= 400;
  const auto html = ui::settingsPage::render(v);
  headers();
  server().send(code, "text/html; charset=utf-8", html.c_str());
}
void showSettings() {
  if (server().hasArg("saved"))
    page(200, "Settings saved. The radar will use them on its next polling cycle.");
  else if (WiFi.status() != WL_CONNECTED)
    page(200, "Connect to your home Wi-Fi using Configure Wi-Fi, then enter your station and radar center here.");
  else if (!location::configured())
    page(200, "Set your radar center and station address to finish setup.");
  else page();
}
void saveSettings() {
  if (server().method() != HTTP_POST) {
    server().sendHeader("Allow", "POST");
    page(405, "Use Save settings to update the radar."); return;
  }
  if (server().arg("csrf") != s_csrf) {
    page(403, "This settings form has expired. Reload the page before saving."); return;
  }
  if (server().arg("revision") != String(s_revision)) {
    page(409, "Settings changed since this form was opened. Reload and review the current values before saving."); return;
  }
  if (!server().hasArg("station_host") || !server().hasArg("radar_lat") ||
      !server().hasArg("radar_lon") || !server().hasArg("station_data_port") ||
      !server().hasArg("station_web_port")) {
    page(400, "The settings form is incomplete. Reload the page and try again."); return;
  }
  const auto v = values(true);
  std::string host;
  uint16_t data = 0, web = 0;
  double lat = 0, lon = 0;
  if (!parseHost(v.host.c_str(), host)) {
    page(400, "Enter a valid station IPv4 address, such as 192.168.1.100, without a URL or port. Leave blank to disconnect.", true); return;
  }
  if (!parsePort(v.dataPort.c_str(), data) || !parsePort(v.webPort.c_str(), web)) {
    page(400, "Both ports must be whole numbers from 1 to 65535.", true); return;
  }
  if (!parseCoordinates(v.lat.c_str(), v.lon.c_str(), lat, lon)) {
    page(400, "Enter a latitude from -90 to 90 and longitude from -180 to 180 in decimal degrees.", true); return;
  }
  const bool changed = station::host() != host.c_str() ||
                       station::dataPort() != data || station::webPort() != web ||
                       location::lat() != lat || location::lon() != lon;
  // Validate the entire form before writing any setting. Receiver fields commit
  // together; legacy location/display keys remain compatible with old firmware.
  if (!station::saveConnection(host.c_str(), data, web)) {
    page(500, "Could not save the station connection. Check the current settings and retry.", true); return;
  }
  if (changed) adsb::configurationChanged();
  ++s_revision;
  if (!location::saveFromStrings(v.lat.c_str(), v.lon.c_str())) {
    page(500, "Station connection saved, but the radar center could not be saved. Review the current settings and retry."); return;
  }
  ui::radar::saveMilesFromPortal(v.miles ? "T" : "");
  ui::radar::saveRunwaysFromPortal(v.runways ? "T" : "");
  headers();
  server().sendHeader("Location", "/param?saved=1");
  server().send(303, "text/plain", "Settings saved");
}
void status() {
  const auto& s = station::status();
  const auto now = millis();
  const bool online = station::online();
  JsonDocument doc;
  doc["state"] = station::host().isEmpty() ? "NOT CONFIGURED" :
                 !location::configured() ? "SET RADAR CENTER" :
                 WiFi.status() != WL_CONNECTED ? "WI-FI DISCONNECTED" :
                 !online ? "OFFLINE" :
                 uint32_t(now - s.feed.advancedAt) >= station::kFeedDelayedMs ? "DELAYED" : "ONLINE";
  if (s.feed.valid) doc["feed_age_s"] = uint32_t(now - s.feed.advancedAt) / 1000;
  else doc["feed_age_s"] = nullptr;
  if (online) doc["tracked"] = s.tracked; else doc["tracked"] = nullptr;
  if (online && s.rateValid && uint32_t(now - s.rateAt) < station::kOfflineAfterMs)
    doc["rate"] = s.messagesPerSecond;
  else doc["rate"] = nullptr;
  if (online && s.temperatureValid && uint32_t(now - s.temperatureAt) < station::kOfflineAfterMs)
    doc["temperature"] = s.temperatureC;
  else doc["temperature"] = nullptr;
  doc["aircraft_url"] = station::host().isEmpty() ? String("") : station::dataUrl("/data/aircraft.json");
  doc["map_coverage"] = ui::map::coverage();
  String json;
  serializeJson(doc, json);
  headers();
  server().send(200, "application/json", json);
}
}  // namespace
void attachPortal(WiFiManager& wm) {
  s_wm = &wm;
  char token[33];
  snprintf(token, sizeof(token), "%08lx%08lx%08lx%08lx",
           (unsigned long)esp_random(), (unsigned long)esp_random(),
           (unsigned long)esp_random(), (unsigned long)esp_random());
  s_csrf = token;
  wm.setTitle("ESP Radar");
  wm.setParamsPage(true); // Wi-Fi saves cannot overwrite the radar settings.
  wm.setWebServerCallback([] {
    server().on("/", HTTP_GET, showSettings);
    server().on("/param", HTTP_GET, showSettings);
    server().on("/settings", HTTP_GET, showSettings);
    server().on("/paramsave", saveSettings);
    server().on("/api/radar/status", HTTP_GET, status);
  });
}
}  // namespace services::settings
