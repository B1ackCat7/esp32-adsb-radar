#include "ui/display_theme.h"

#include <cassert>
#include <iostream>
int main() {
  using namespace ui::theme;
  assert(temperatureColor(59.9f, true) == cyan);
  assert(temperatureColor(60.0f, true) == orange);
  assert(temperatureColor(79.9f, true) == orange);
  assert(temperatureColor(80.0f, true) == orange);
  assert(temperatureColor(80.1f, true) == danger);
  assert(temperatureColor(90.0f, false) == muted);
  assert(temperatureColor(40.0f, false) == muted);
  std::cout << "Temperature boundaries and unavailable values passed\n";
}
