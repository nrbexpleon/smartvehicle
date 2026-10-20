#pragma once
#include "smartvehicle/platform/api.hpp"
#include "smartvehicle/runtime/executor.hpp"
#include <chrono>
#include <map>
#include <shared_mutex>
#include <string>
#include <vector>

namespace smartvehicle {
enum class ComponentState { starting, healthy, degraded, failed, isolated };
enum class RecoveryAction { none, restart_component, enter_degraded_mode, request_safe_state };

struct ComponentPolicy {
  std::chrono::milliseconds heartbeat_timeout{1000};
  std::uint32_t restart_limit{3};
  Criticality criticality{Criticality::quality_management};
};

struct ComponentHealth {
  std::string name;
  ComponentState state{ComponentState::starting};
  RecoveryAction action{RecoveryAction::none};
  std::uint32_t restart_count{0};
  std::string detail;
};

struct PlatformHealthReport {
  ComponentState aggregate{ComponentState::healthy};
  bool safe_state_requested{false};
  std::vector<ComponentHealth> components;
};

class HealthSupervisor {
 public:
  using TimePoint = std::chrono::steady_clock::time_point;
  Result<bool> register_component(std::string name, ComponentPolicy policy, TimePoint now);
  Result<bool> heartbeat(std::string_view name, bool healthy, std::string detail, TimePoint now);
  Result<bool> acknowledge_restart(std::string_view name, TimePoint now);
  [[nodiscard]] PlatformHealthReport evaluate(TimePoint now);

 private:
  struct Entry { ComponentPolicy policy; ComponentHealth health; TimePoint last_heartbeat; };
  mutable std::shared_mutex mutex_;
  std::map<std::string, Entry, std::less<>> entries_;
};
}  // namespace smartvehicle
