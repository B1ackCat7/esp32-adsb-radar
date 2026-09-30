#pragma once
#include <cstdint>
namespace services {
inline bool deadlineReached(uint32_t now, uint32_t due) {
  return int32_t(now - due) >= 0;
}
inline void advanceDeadline(uint32_t& due, uint32_t now, uint32_t period) {
  if (deadlineReached(now, due)) due += ((now - due) / period + 1) * period;
}
// All requests share one clock; I/O duration does not shift the next cycle.
struct PollSchedule {
  enum Task { None = -1, Aircraft = 0, Stats = 1, Temperature = 2 };
  static constexpr uint32_t kCycleMs = 10000;
  uint32_t due[3]{};
  uint32_t finishedAt = 0;
  bool finished = false, forceAircraft = false;
  void reset(uint32_t now) {
    due[Aircraft] = now;
    due[Stats] = now + 3000;
    due[Temperature] = now + 6000;
    finished = false;
    forceAircraft = false;
  }
  Task next(uint32_t now, bool metricsAllowed) const {
    if (finished && uint32_t(now - finishedAt) < 250) return None;
    if (forceAircraft || deadlineReached(now, due[Aircraft])) return Aircraft;
    if (!metricsAllowed) return None;
    if (deadlineReached(now, due[Stats])) return Stats;
    if (deadlineReached(now, due[Temperature])) return Temperature;
    return None;
  }
  void begin(Task task, uint32_t now) {
    advanceDeadline(due[task], now, kCycleMs);
    if (task == Aircraft) forceAircraft = false;
  }
  void complete(uint32_t now) {
    finishedAt = now;
    finished = true;
  }
};
struct DisplayCycle {
  uint32_t due = 0;
  void reset(uint32_t now) { due = now + PollSchedule::kCycleMs; }
  bool poll(uint32_t now) {
    if (!deadlineReached(now, due)) return false;
    advanceDeadline(due, now, PollSchedule::kCycleMs);
    return true;
  }
};
}  // namespace services
