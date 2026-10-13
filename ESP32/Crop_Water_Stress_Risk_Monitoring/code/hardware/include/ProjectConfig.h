#pragma once
#include <stddef.h>
#include <stdint.h>
namespace cws {
constexpr uint32_t SetupWindowMs = 300000;
constexpr size_t MaxMessage = 8192;
constexpr size_t MaxHttpBody = 12288;
constexpr size_t MaxRecords = 256;
constexpr uint32_t PasswordIterations = 60000;
} // namespace cws
