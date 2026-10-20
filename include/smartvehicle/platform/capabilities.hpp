#pragma once
#include "smartvehicle/platform/api.hpp"
#include <cstddef>
#include <string>
#include <vector>

namespace smartvehicle {
enum class OsFamily { linux, posix_rtos, freertos, zephyr, unknown };
enum class CpuArchitecture { x86_64, arm64, arm32, riscv64, unknown };

struct PlatformCapabilities {
  OsFamily os{OsFamily::unknown};
  CpuArchitecture architecture{CpuArchitecture::unknown};
  std::size_t logical_cores{1};
  std::size_t memory_budget_bytes{0};
  bool monotonic_clock{false};
  bool threads{false};
  bool virtual_memory{false};
  bool hardware_crypto{false};
  bool can{false};
  bool ethernet{false};
};

struct WorkloadRequirements {
  std::size_t minimum_cores{1};
  std::size_t minimum_memory_bytes{0};
  bool needs_threads{false};
  bool needs_virtual_memory{false};
  bool needs_hardware_crypto{false};
  bool needs_can{false};
  bool needs_ethernet{false};
};

struct CompatibilityReport {
  bool compatible{false};
  std::vector<std::string> unmet;
};

[[nodiscard]] CompatibilityReport evaluate_capabilities(const PlatformCapabilities&, const WorkloadRequirements&);
[[nodiscard]] PlatformCapabilities detect_host_capabilities();
[[nodiscard]] std::string_view to_string(OsFamily) noexcept;
[[nodiscard]] std::string_view to_string(CpuArchitecture) noexcept;
}  // namespace smartvehicle
