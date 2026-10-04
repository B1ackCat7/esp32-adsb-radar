#pragma once
#include "../misc/DataWrapper.hpp"
#include "../misc/enum.hpp"
#include "../../utility/result.hpp"
#include <cstdlib>
#include <cstdio>

// Headless host adapter only. LovyanGFX's rasterization/font code is unchanged.
namespace lgfx { inline namespace v1 {
inline unsigned long millis() { return 100000; }
inline unsigned long micros() { return 100000000; }
inline void delay(unsigned long) {}
inline void delayMicroseconds(unsigned int) {}
inline void* heap_alloc(size_t n) { return std::malloc(n); }
inline void* heap_alloc_psram(size_t n) { return std::malloc(n); }
inline void* heap_alloc_dma(size_t n) { return std::malloc(n); }
inline void heap_free(void* p) { std::free(p); }
inline bool heap_capable_dma(const void*) { return false; }
inline void gpio_hi(uint32_t) {}
inline void gpio_lo(uint32_t) {}
inline bool gpio_in(uint32_t) { return false; }
enum pin_mode_t { output, input, input_pullup, input_pulldown };
inline void pinMode(int_fast16_t, pin_mode_t) {}
inline void lgfxPinMode(int_fast16_t, pin_mode_t) {}
struct FileWrapper : public DataWrapper {
  FILE* fp = nullptr;
  FileWrapper() { need_transaction = false; }
  bool open(const char* p) override { return (fp = std::fopen(p, "rb")); }
  int read(uint8_t* p, uint32_t n) override { return std::fread(p, 1, n, fp); }
  void skip(int32_t n) override { std::fseek(fp, n, SEEK_CUR); }
  bool seek(uint32_t n) override { return std::fseek(fp, n, SEEK_SET) == 0; }
  void close() override { if (fp) std::fclose(fp); fp = nullptr; }
  int32_t tell() override { return std::ftell(fp); }
};
}}
