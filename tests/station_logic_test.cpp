#include "services/station_logic.h"

#include <cassert>
#include <iostream>
#include <limits>

#include "ui/page_cycle.h"
#include "ui/radar_range.h"
using namespace services::station;
int main() {
  ui::Pages pages;
  assert(pages.current == 0 && pages.lastRadar == 0);
  for (int round = 0; round < 2; ++round) {
    for (int i = 1; i <= 4; ++i) {
      pages.next();
      assert(pages.current == i % 4);
      assert(pages.lastRadar == (i == 3 ? 2 : i % 4));
    }
  }
  pages.select(3);
  pages.nextRadar();
  assert(pages.current == 1);
  pages.nextRadar();
  assert(pages.current == 2);
  pages.nextRadar();
  assert(pages.current == 0);
  pages.select(9);
  assert(pages.current == 0);
  static_assert(ui::radar::kRangePresetCount == ui::Pages::kRadarCount);
  assert(ui::radar::kRangePresets[0].ring3_km == 25);
  assert(ui::radar::kRangePresets[1].ring3_km == 50);
  assert(ui::radar::kRangePresets[2].ring3_km == 100);
  ui::PageCycle cycle;
  cycle.restart(100);
  assert(!cycle.poll(25099));
  assert(cycle.poll(25100));
  assert(!cycle.poll(25101));
  assert(cycle.poll(50100));
  cycle.restart(60000);  // manual selection grants a full new interval
  assert(!cycle.poll(84999));
  assert(cycle.poll(85000));
  cycle.restart(0xfffffff0u);
  assert(!cycle.poll(0xfffffff0u + 24999));
  assert(cycle.poll(0xfffffff0u + 25000));
  assert(cycle.poll(
      100000));  // delayed loop changes once, without catch-up flicker
  assert(!cycle.poll(100000));
  Freshness f;
  assert(!f.online(0));
  assert(!f.accept(std::numeric_limits<double>::quiet_NaN(), 0));
  assert(f.accept(100, 100));
  // A failed poll, including Wi-Fi loss, must not instantly flip ONLINE.
  f.reachable = false;
  assert(f.online(2100));
  assert(f.online(30099));
  assert(!f.online(30100));
  // Repeated failures and malformed timestamps cannot extend the grace window.
  assert(!f.accept(std::numeric_limits<double>::quiet_NaN(), 30101));
  assert(!f.online(30101));
  assert(f.accept(101, 31000));
  assert(!f.accept(-1, 32000));
  assert(f.online(32000));
  assert(f.accept(102, 33000));
  assert(f.online(62999));
  assert(!f.online(63000));
  // Successful HTTP with an unchanged timestamp is still a frozen feed.
  assert(!f.accept(102, 63001));
  assert(!f.online(63001));
  assert(f.accept(103, 63002));
  assert(f.accept(1, 63003));  // readsb clock restart
  f = {};
  assert(f.accept(1, 0xfffffff0));
  f.reachable = false;
  assert(f.online(0x100));
  assert(f.online(0xfffffff0u + kOfflineAfterMs - 1));
  assert(!f.online(0xfffffff0u + kOfflineAfterMs));
  assert(localType("adsb_icao"));
  assert(localType("mode_s"));
  assert(!localType("mlat"));
  assert(!localType("tisb_icao"));
  assert(!localType(nullptr));
  assert(distanceKm(0, 0, 0, 0) == 0);
  assert(std::abs(distanceKm(0, 0, 0, 1) - 111.195) < 0.01);
  Clicks c;
  assert(c.release(100) == 0);
  assert(c.poll(450) == 0);
  assert(c.poll(451) == 1);
  assert(c.release(500) == 0);
  assert(c.release(700) == 2);
  assert(c.poll(1200) == 0);
  assert(c.release(2000) == 0);
  assert(c.release(2500) == 1);
  assert(c.poll(2900) == 1);
  c = {};
  assert(c.release(0xfffffff0) == 0);
  assert(c.release(100) == 2);
  std::cout << "Freshness, recovery, clock reset, rollover, source filtering, "
               "distance and button tests passed\n";
}
