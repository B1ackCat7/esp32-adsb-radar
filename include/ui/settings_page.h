#pragma once

#include <string>
#include "services/settings_validation.h"

namespace ui::settingsPage {
struct Values {
  std::string host, dataPort, webPort, lat, lon, csrf, revision, firmware, ip;
  std::string message;
  bool miles = false, runways = true, error = false;
};

// Shared by the firmware and native preview: no external fonts, scripts or CSS.
inline const char* pageTemplate() {
  return R"HTML(<!doctype html>
<html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width, initial-scale=1">
<title>ESP Radar settings</title><style>
:root{color-scheme:dark;font-family:system-ui,sans-serif;background:#050e20;color:#e3f2fa;line-height:1.5}*{box-sizing:border-box}body{max-width:760px;margin:auto;padding:24px 18px 40px}h1{font-size:2rem;margin:0}h2{font-size:1.2rem;margin:0 0 12px}p{margin:10px 0}a{color:#51c3e1}header{margin-bottom:22px}.eyebrow{color:#ff9d3d;letter-spacing:.12em;font-size:.8rem;font-weight:700}.muted,small{color:#7ca5bc}section{background:#0a1b31;border:1px solid #174465;border-radius:14px;padding:20px;margin:18px 0}label{display:block;font-weight:600;margin-top:16px}input:not([type=checkbox]){width:100%;padding:12px;border:1px solid #1e4d66;border-radius:8px;background:#030c1d;color:#e3f2fa;font:inherit;margin:6px 0}input:focus,button:focus,a:focus{outline:2px solid #51c3e1;outline-offset:3px}input[type=checkbox]{width:20px;height:20px;accent-color:#ff9d3d;margin:0 10px 0 0}.check{display:flex;align-items:center}.grid{display:grid;grid-template-columns:1fr 1fr;gap:18px}button,.button{display:inline-block;background:#ff9d3d;color:#050e20;border:0;border-radius:8px;padding:12px 18px;font:inherit;font-weight:700;cursor:pointer;text-decoration:none}.secondary{background:#143149;color:#e3f2fa}nav{display:flex;gap:18px;flex-wrap:wrap;margin-top:14px}.notice{border-left:3px solid #51c3e1;padding:12px;background:#0a1b31}.notice:empty{display:none}.error{border-color:#ff6165}.status{color:#51c3e1;font-weight:700}.status.offline{color:#ff6165}.status.delayed{color:#ff9d3d}dl{display:grid;grid-template-columns:145px 1fr;gap:6px;margin:12px 0}dt{color:#7ca5bc}dd{margin:0;overflow-wrap:anywhere}summary{cursor:pointer;color:#51c3e1;margin-top:18px}details small{display:block}code{overflow-wrap:anywhere}footer{font-size:.85rem;color:#7ca5bc}@media(max-width:500px){.grid{grid-template-columns:1fr;gap:0}dl{grid-template-columns:110px 1fr}section{padding:16px}h1{font-size:1.7rem}}
</style></head><body>
<header><div class="eyebrow">ESP RADAR</div><h1>Radar settings</h1><p class="muted">Connect your radar to the ADSB.im station on your home network.</p>
<nav><a href="/wifi">Configure Wi-Fi</a><a href="/info">Device information</a></nav></header>
<p class="notice {{errorClass}}" role="status">{{message}}</p>
<section aria-labelledby="connection-title"><h2 id="connection-title">Connection status</h2>
<p id="feed-status" class="status" role="status">Checking saved connection…</p>
<dl><dt>Radar address</dt><dd><a href="http://{{ip}}/">{{ip}}</a></dd><dt>Settings name</dt><dd><a href="http://plane-radar.local/">plane-radar.local</a></dd><dt>Firmware</dt><dd>{{firmware}}</dd><dt>Last feed advance</dt><dd id="feed-age">—</dd><dt>Aircraft tracked</dt><dd id="tracked">—</dd><dt>Message rate</dt><dd id="rate">—</dd><dt>Pi temperature</dt><dd id="temperature">—</dd></dl>
<button type="button" class="secondary" id="refresh-status">Refresh status</button>
<p><small>Status uses the radar’s saved connection and normal ten-second polling. A healthy empty feed is online with zero aircraft. Missing optional metrics do not mean the aircraft feed is offline.</small></p>
</section>
<form action="/paramsave" method="post">
<input type="hidden" name="csrf" value="{{csrf}}"><input type="hidden" name="revision" value="{{revision}}">
<section aria-labelledby="station-title"><h2 id="station-title">Your ADSB.im station</h2>
<label for="station_host">Station IPv4 address</label><input id="station_host" name="station_host" value="{{host}}" maxlength="63" inputmode="decimal" autocomplete="off" spellcheck="false" placeholder="192.168.1.100" aria-describedby="host-help">
<small id="host-help">Enter the station’s address, not the radar’s. Address only: no http://, hostname, port or path. Find it in your router’s connected-device list. Leave blank to disconnect the station.</small>
<p><small>Keep both devices on networks that can reach each other. A router DHCP reservation keeps your station’s address stable.</small></p>
<details><summary>Advanced connection settings</summary>
<div class="grid"><div><label for="station_data_port">Aircraft / statistics port</label><input type="number" id="station_data_port" name="station_data_port" value="{{dataPort}}" min="1" max="65535" step="1" required><small>Usually 8080.</small></div>
<div><label for="station_web_port">ADSB.im web / temperature port</label><input type="number" id="station_web_port" name="station_web_port" value="{{webPort}}" min="1" max="65535" step="1" required><small>Usually 80; some installations use 1099.</small></div></div>
<p><small>Change these only if your station uses different HTTP ports. The paths remain /data/aircraft.json, /data/stats.json and /api/get_temperatures.json.</small></p></details>
</section>
<section aria-labelledby="location-title"><h2 id="location-title">Radar center</h2><p class="muted">Enter your station or nearby viewing location in decimal degrees. A nearby approximate center is sufficient.</p>
<div class="grid"><div><label for="radar_lat">Latitude</label><input type="number" id="radar_lat" name="radar_lat" value="{{lat}}" min="-90" max="90" step="any" required><small>−90 to 90; north is positive.</small></div>
<div><label for="radar_lon">Longitude</label><input type="number" id="radar_lon" name="radar_lon" value="{{lon}}" min="-180" max="180" step="any" required><small>−180 to 180; west is negative.</small></div></div>
<p><small>Saving new coordinates automatically rebuilds the base map on the radar. World coastlines and major lakes are included. No internet connection, download or map account is needed. Small features and streets are not included in the world map. Polar areas at or beyond 85° latitude show range rings only.</small></p>
<p><small>Base map: <span id="map-coverage">Checking map…</span></small></p></section>
<section aria-labelledby="display-title"><h2 id="display-title">Display</h2>
<label class="check" for="use_miles"><input type="checkbox" id="use_miles" name="use_miles" value="T" {{miles}}>Display distances in miles</label>
<label class="check" for="show_runways"><input type="checkbox" id="show_runways" name="show_runways" value="T" {{runways}}>Show airport runways</label>
<p><small>25 / 50 / 100 km Radar and Station rotate every 25 seconds. Aircraft text appears on the closest range only.</small></p></section>
<button type="submit">Save settings</button><p><small>Changes apply without restarting. Allow up to one polling cycle for new station data.</small></p></form>
<section><h2>Connection help</h2><p>If the feed is offline, check the address and ports, Wi-Fi, and any guest-network isolation. Open the saved aircraft endpoint below to check whether the station exposes JSON.</p><p id="endpoint"></p><p><small>If .local does not resolve on your phone or computer, open the radar’s IP address instead. Holding BOOT for three seconds resets Wi-Fi, center and units; it keeps the saved station connection.</small></p></section>
<footer>Settings are intended for your trusted home network. No account or cloud aircraft service is required.</footer>
<script>
(()=>{let busy=false;const byId=id=>document.getElementById(id);async function refresh(){if(busy)return;busy=true;try{const response=await fetch('/api/radar/status',{cache:'no-store'});if(!response.ok)throw Error();const s=await response.json();const label=byId('feed-status');label.textContent=s.state;label.className='status '+(s.state==='OFFLINE'?'offline':s.state==='DELAYED'?'delayed':'');byId('feed-age').textContent=s.feed_age_s===null?'No valid feed yet':s.feed_age_s+' seconds ago';byId('map-coverage').textContent=s.map_coverage||'Map status unavailable';byId('tracked').textContent=s.tracked===null?'—':s.tracked;byId('rate').textContent=s.rate===null?'—':s.rate.toFixed(1)+' messages/s';byId('temperature').textContent=s.temperature===null?'—':s.temperature.toFixed(1)+' °C';const endpoint=byId('endpoint');endpoint.replaceChildren();if(s.aircraft_url){const link=document.createElement('a');link.href=s.aircraft_url;link.textContent=s.aircraft_url;link.target='_blank';link.rel='noopener';endpoint.append(link);}else endpoint.textContent='Enter your station address and save to begin.';}catch(e){byId('feed-status').textContent='Radar status unavailable — check your Wi-Fi connection.';byId('feed-status').className='status offline';for(const id of ['feed-age','tracked','rate','temperature','map-coverage'])byId(id).textContent='—';}finally{busy=false;}}byId('refresh-status').addEventListener('click',refresh);refresh();setInterval(()=>{if(!document.hidden)refresh();},10000);})();
</script></body></html>)HTML";
}

inline std::string render(const Values& v) {
  using services::settings::escapeHtml;
  const std::string source = pageTemplate();
  std::string out;
  out.reserve(source.size() + 512);
  size_t pos = 0;
  while (pos < source.size()) {
    const auto start = source.find("{{", pos);
    if (start == std::string::npos) { out.append(source, pos); break; }
    out.append(source, pos, start - pos);
    const auto end = source.find("}}", start);
    const auto key = source.substr(start + 2, end - start - 2);
    if (key == "host") out += escapeHtml(v.host);
    else if (key == "dataPort") out += escapeHtml(v.dataPort);
    else if (key == "webPort") out += escapeHtml(v.webPort);
    else if (key == "lat") out += escapeHtml(v.lat);
    else if (key == "lon") out += escapeHtml(v.lon);
    else if (key == "csrf") out += escapeHtml(v.csrf);
    else if (key == "revision") out += escapeHtml(v.revision);
    else if (key == "firmware") out += escapeHtml(v.firmware);
    else if (key == "ip") out += escapeHtml(v.ip);
    else if (key == "message") out += escapeHtml(v.message);
    else if (key == "errorClass") out += v.error ? "error" : "";
    else if (key == "miles") out += v.miles ? "checked" : "";
    else if (key == "runways") out += v.runways ? "checked" : "";
    pos = end + 2;
  }
  return out;
}
}  // namespace ui::settingsPage
