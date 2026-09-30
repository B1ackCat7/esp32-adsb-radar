#include "services/settings_validation.h"
#include "ui/settings_page.h"

#include <cassert>
#include <fstream>
#include <iostream>
#include <string>

using namespace services::settings;
int main(int argc, char** argv) {
  std::string host;
  assert(parseHost(" 192.168.1.100 ", host) && host == "192.168.1.100");
  assert(parseHost("10.0.0.2", host) && host == "10.0.0.2");
  assert(parseHost("172.16.1.4", host));
  assert(parseHost("", host) && host.empty());
  assert(parseHost(nullptr, host) && host.empty());
  for (auto bad : {"192.168.1", "256.1.1.1", "1.2.3.4.5", "http://10.0.0.2",
                   "10.0.0.2:8080", "10.0.0.2/data", "adsb-feeder.local",
                   "0.0.0.0", "127.0.0.1", "224.0.0.1", "255.255.255.255",
                   "-1.2.3.4", "1..3.4", "0001.2.3.4", "<script>", "1.2.3.4x"})
    assert(!parseHost(bad, host));
  uint16_t port;
  assert(parsePort("8080", port) && port == 8080);
  assert(parsePort(" 1099 ", port) && port == 1099);
  assert(parsePort("1", port) && port == 1);
  assert(parsePort("65535", port) && port == 65535);
  for (auto bad : {"", "0", "65536", "-1", "+80", "80.0", "8e3", "80x", "99999999"})
    assert(!parsePort(bad, port));
  double lat, lon;
  assert(parseCoordinates("0", "0", lat, lon));
  assert(parseCoordinates("-90", "180", lat, lon));
  assert(parseCoordinates(" 0.000000 ", "0.000000", lat, lon));
  for (auto bad : {"", "nan", "inf", "1e999", "91", "-91", "12x"})
    assert(!parseCoordinates(bad, "0", lat, lon));
  assert(!parseCoordinates("0", "181", lat, lon));
  assert(!parseCoordinates("0", "-181", lat, lon));
  assert(escapeHtml("<&\"'>") == "&lt;&amp;&quot;&#39;&gt;");
  ui::settingsPage::Values v;
  v.host = "192.168.1.100"; v.dataPort = "8080"; v.webPort = "80";
  v.lat = "0.000000"; v.lon = "0.000000";
  v.csrf = "preview-token"; v.revision = "1"; v.ip = "192.168.1.101";
  v.firmware = "0.3.0-beta.1";
  auto html = ui::settingsPage::render(v);
  assert(html.find("{{") == std::string::npos);
  assert(html.find("name=\"station_host\" value=\"192.168.1.100\"") != std::string::npos);
  assert(html.find("/paramsave\" method=\"post\"") != std::string::npos);
  assert(html.find("name=\"show_runways\" value=\"T\" checked") != std::string::npos);
  assert(html.find("name=\"use_miles\" value=\"T\" checked") == std::string::npos);
  assert(html.find("automatically rebuilds the base map") != std::string::npos);
  assert(html.find("id=\"map-coverage\"") != std::string::npos);
  if (argc > 1) std::ofstream(argv[1]) << html;
  v.host = "\"><script>alert(1)</script>{{csrf}}";
  const auto malicious = ui::settingsPage::render(v);
  assert(malicious.find("<script>alert(1)</script>") == std::string::npos);
  assert(malicious.find("&lt;script&gt;") != std::string::npos);
  assert(malicious.find("{{csrf}}") != std::string::npos); // input never becomes a template
  v.host = ""; v.lat = ""; v.lon = "";
  const auto fresh = ui::settingsPage::render(v);
  assert(fresh.find("name=\"station_host\" value=\"\"") != std::string::npos);
  assert(fresh.find("name=\"radar_lat\" value=\"\"") != std::string::npos);
  std::cout << "Settings validation, safe rendering and first-install form passed\n";
}
