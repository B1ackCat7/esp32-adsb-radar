#include "services/poll_schedule.h"

#include <cassert>
using S = services::PollSchedule;
int main() {
  S s;
  s.reset(100);
  assert(s.next(100, false) == S::Aircraft);
  s.begin(S::Aircraft, 100);
  s.complete(500);
  assert(s.next(749, true) == S::None);
  assert(s.next(3099, true) == S::None);
  assert(s.next(3100, true) == S::Stats);
  s.begin(S::Stats, 3500);
  s.complete(4200);
  assert(s.due[S::Stats] ==
         13100);  // completion and start lateness do not shift phase
  assert(s.next(6100, true) == S::Temperature);
  s.begin(S::Temperature, 6100);
  s.complete(12000);
  assert(s.next(12249, true) == S::None);
  assert(s.next(12250, true) == S::Aircraft);
  s.begin(S::Aircraft, 12250);
  s.complete(12500);
  assert(s.due[S::Aircraft] == 20100);
  assert(s.next(13100, false) == S::None);
  assert(s.next(13100, true) == S::Stats);
  s.begin(S::Stats, 43100);
  s.complete(43500);  // skip missed slots, never burst catch-up
  assert(s.due[S::Stats] == 53100);
  services::DisplayCycle d;
  d.reset(100);
  assert(!d.poll(10099));
  assert(d.poll(10100));
  assert(!d.poll(10101));
  assert(d.poll(21500));
  assert(d.due == 30100);
  assert(d.poll(60101));
  assert(d.due == 70100);
  S w;
  w.reset(0xfffffff0u);
  w.begin(S::Aircraft, 0xfffffff0u);
  w.complete(0xfffffff0u);
  assert(w.next(0xfffffff0u + 10000, false) == S::Aircraft);
  d.reset(0xfffffff0u);
  assert(!d.poll(0xfffffff0u + 9999));
  assert(d.poll(0xfffffff0u + 10000));
  s.reset(100);
  s.forceAircraft = true;
  s.begin(S::Aircraft, 100);
  s.complete(200);
  s.forceAircraft = true;
  assert(s.next(500, true) == S::Aircraft);
  s.begin(S::Aircraft, 500);
  assert(s.due[S::Aircraft] == 10100);  // manual fetch doesn't reset the clock
}
