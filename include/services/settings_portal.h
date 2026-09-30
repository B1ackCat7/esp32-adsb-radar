#pragma once
class WiFiManager;
namespace services::settings {
/** Register routes before WiFiManager's defaults, on AP and LAN alike. */
void attachPortal(WiFiManager& wm);
}
