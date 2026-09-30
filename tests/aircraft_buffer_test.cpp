#include "services/aircraft_buffer.h"

#include <cassert>
#include <cstdint>

#include "services/adsb_client.h"
struct Target {
  char hex[9];
  uint32_t observedAt;
  float positionAge;
};
using Buffer = services::adsb::AircraftBuffer<Target, 2>;
int main() {
  Target zero = {"zero", 100, 0};
  assert(Buffer::alive(zero, 30099));
  assert(!Buffer::alive(zero, 30100));
  services::adsb::Aircraft ac{};
  ac.observedAt = 0xfffffff0u;
  ac.positionAge = 10;
  assert(!services::adsb::heldPosition(ac, 0xfffffff0u + 4999));
  assert(services::adsb::heldPosition(ac, 0xfffffff0u + 5000));
  Buffer b;
  Target a[2] = {{"abc", 100, 2}};
  assert(b.merge(a, 1, 100) == 1);
  assert(b.merge(a, 0, 2100) == 1);
  assert(b.merge(a, 0, 8500) == 1);
  assert(b.merge(a, 0, 21000) ==
         1);  // two missed ten-second snapshots // short dropout
  assert(b.merge(a, 0, 28099) == 1);
  assert(b.merge(a, 0, 28100) == 0);  // cannot renew retention
  a[0] = {"abc", 100, 10};
  assert(b.merge(a, 1, 100) == 1);
  assert(b.merge(a, 0, 20099) == 1);
  assert(b.merge(a, 0, 20100) == 0);  // total position age cap
  a[0] = {"abc", 0xfffffff0u, 0};
  b.merge(a, 1, 0xfffffff0u);
  assert(b.merge(a, 0, 100) == 1);  // millis wrap
  b.remove("abc");
  assert(b.merge(a, 0, 101) == 0);  // explicit exclusion
  a[0] = {"abc", 100, 0};
  b.merge(a, 1, 100);
  a[0] = {"def", 200, 0};
  a[1] = {"ghi", 200, 0};
  assert(b.merge(a, 2, 200) == 2);  // fresh targets win capacity
  assert(a[0].hex[0] == 'd' && a[1].hex[0] == 'g');
  b.clear();
  assert(b.merge(a, 0, 201) == 0);
  a[0] = {"abc", 300, 0};
  b.merge(a, 1, 300);
  a[0] = {"abc", 2300, 1};
  assert(b.merge(a, 1, 2300) == 1);  // reappearance updates, no duplicate
  assert(a[0].observedAt == 2300 && a[0].positionAge == 1);
  assert(b.merge(a, 0, 31299) == 1);
  assert(b.merge(a, 0, 31300) == 0);
}
