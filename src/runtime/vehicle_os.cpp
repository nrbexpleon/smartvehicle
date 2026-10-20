#include "smartvehicle/platform/capabilities.hpp"
#include "smartvehicle/runtime/executor.hpp"
#include "smartvehicle/runtime/supervisor.hpp"
#include <algorithm>
#include <thread>

namespace smartvehicle {
CompatibilityReport evaluate_capabilities(const PlatformCapabilities& c, const WorkloadRequirements& r) {
  CompatibilityReport report;
  if (c.logical_cores < r.minimum_cores) report.unmet.emplace_back("logical_cores");
  if (c.memory_budget_bytes < r.minimum_memory_bytes) report.unmet.emplace_back("memory_budget_bytes");
  if (r.needs_threads && !c.threads) report.unmet.emplace_back("threads");
  if (r.needs_virtual_memory && !c.virtual_memory) report.unmet.emplace_back("virtual_memory");
  if (r.needs_hardware_crypto && !c.hardware_crypto) report.unmet.emplace_back("hardware_crypto");
  if (r.needs_can && !c.can) report.unmet.emplace_back("can");
  if (r.needs_ethernet && !c.ethernet) report.unmet.emplace_back("ethernet");
  if (!c.monotonic_clock) report.unmet.emplace_back("monotonic_clock");
  report.compatible = report.unmet.empty();
  return report;
}

PlatformCapabilities detect_host_capabilities() {
  PlatformCapabilities value;
#if defined(__linux__)
  value.os = OsFamily::linux;
#elif defined(__unix__)
  value.os = OsFamily::posix_rtos;
#endif
#if defined(__x86_64__) || defined(_M_X64)
  value.architecture = CpuArchitecture::x86_64;
#elif defined(__aarch64__)
  value.architecture = CpuArchitecture::arm64;
#elif defined(__arm__)
  value.architecture = CpuArchitecture::arm32;
#elif defined(__riscv) && __riscv_xlen == 64
  value.architecture = CpuArchitecture::riscv64;
#endif
  value.logical_cores = std::max(1u, std::thread::hardware_concurrency());
  value.memory_budget_bytes = 64U * 1024U * 1024U;
  value.monotonic_clock = true;
  value.threads = true;
  value.virtual_memory = true;
  value.ethernet = true;
  return value;
}

std::string_view to_string(OsFamily value) noexcept {
  switch (value) { case OsFamily::linux: return "linux"; case OsFamily::posix_rtos: return "posix-rtos";
    case OsFamily::freertos: return "freertos"; case OsFamily::zephyr: return "zephyr"; default: return "unknown"; }
}
std::string_view to_string(CpuArchitecture value) noexcept {
  switch (value) { case CpuArchitecture::x86_64: return "x86-64"; case CpuArchitecture::arm64: return "arm64";
    case CpuArchitecture::arm32: return "arm32"; case CpuArchitecture::riscv64: return "riscv64"; default: return "unknown"; }
}

DeterministicExecutor::DeterministicExecutor(std::size_t maximum_tasks) : maximum_tasks_(maximum_tasks) {
  tasks_.reserve(maximum_tasks);
}
Result<std::size_t> DeterministicExecutor::register_periodic(TaskSpec spec, TimePoint first_release) {
  if (tasks_.size() >= maximum_tasks_) return Result<std::size_t>::failure(ErrorCode::queue_full, "task table full");
  if (spec.name.empty() || spec.period <= std::chrono::milliseconds::zero() ||
      spec.budget <= std::chrono::microseconds::zero() || !spec.callback)
    return Result<std::size_t>::failure(ErrorCode::invalid_argument, "valid name, period, budget and callback required");
  if (std::any_of(tasks_.begin(), tasks_.end(), [&](const Task& task) { return task.spec.name == spec.name; }))
    return Result<std::size_t>::failure(ErrorCode::already_exists, "task already registered");
  const auto index = tasks_.size();
  tasks_.push_back({std::move(spec), first_release, {}, index});
  return Result<std::size_t>::success(index);
}
DispatchSummary DeterministicExecutor::run_ready(TimePoint now, std::size_t maximum_dispatches) {
  DispatchSummary summary;
  std::vector<Task*> ready;
  ready.reserve(tasks_.size());
  for (auto& task : tasks_) if (task.next_release <= now) ready.push_back(&task);
  std::stable_sort(ready.begin(), ready.end(), [](const Task* a, const Task* b) {
    if (a->next_release != b->next_release) return a->next_release < b->next_release;
    if (a->spec.criticality != b->spec.criticality) return a->spec.criticality > b->spec.criticality;
    return a->order < b->order;
  });
  summary.deferred = ready.size() > maximum_dispatches ? ready.size() - maximum_dispatches : 0;
  for (std::size_t i = 0; i < ready.size() && i < maximum_dispatches; ++i) {
    auto& task = *ready[i];
    const auto execution = task.spec.callback();
    ++task.stats.dispatches;
    task.stats.maximum_consumed = std::max(task.stats.maximum_consumed, execution.consumed);
    if (execution.consumed > task.spec.budget) { ++task.stats.budget_overruns; ++summary.budget_overruns; }
    if (!execution.healthy) {
      ++task.stats.unhealthy_runs;
      if (task.spec.criticality != Criticality::quality_management) summary.safety_task_unhealthy = true;
    }
    do { task.next_release += task.spec.period; } while (task.next_release <= now);
    ++summary.dispatched;
  }
  return summary;
}
Result<TaskStatistics> DeterministicExecutor::statistics(std::string_view name) const {
  const auto found = std::find_if(tasks_.begin(), tasks_.end(), [&](const Task& task) { return task.spec.name == name; });
  return found == tasks_.end() ? Result<TaskStatistics>::failure(ErrorCode::not_found, "task not found")
                               : Result<TaskStatistics>::success(found->stats);
}
std::size_t DeterministicExecutor::task_count() const noexcept { return tasks_.size(); }

Result<bool> HealthSupervisor::register_component(std::string name, ComponentPolicy policy, TimePoint now) {
  if (name.empty() || policy.heartbeat_timeout <= std::chrono::milliseconds::zero())
    return Result<bool>::failure(ErrorCode::invalid_argument, "component name and timeout required");
  std::unique_lock lock(mutex_);
  if (entries_.contains(name)) return Result<bool>::failure(ErrorCode::already_exists, "component already registered");
  ComponentHealth health{name, ComponentState::starting, RecoveryAction::none, 0, "awaiting heartbeat"};
  entries_.emplace(name, Entry{policy, std::move(health), now});
  return Result<bool>::success(true);
}
Result<bool> HealthSupervisor::heartbeat(std::string_view name, bool healthy, std::string detail, TimePoint now) {
  std::unique_lock lock(mutex_);
  auto found = entries_.find(name);
  if (found == entries_.end()) return Result<bool>::failure(ErrorCode::not_found, "component not registered");
  found->second.last_heartbeat = now;
  found->second.health.state = healthy ? ComponentState::healthy : ComponentState::degraded;
  found->second.health.action = healthy ? RecoveryAction::none : RecoveryAction::enter_degraded_mode;
  found->second.health.detail = std::move(detail);
  return Result<bool>::success(true);
}
Result<bool> HealthSupervisor::acknowledge_restart(std::string_view name, TimePoint now) {
  std::unique_lock lock(mutex_);
  auto found = entries_.find(name);
  if (found == entries_.end()) return Result<bool>::failure(ErrorCode::not_found, "component not registered");
  ++found->second.health.restart_count;
  found->second.last_heartbeat = now;
  found->second.health.state = ComponentState::starting;
  found->second.health.action = RecoveryAction::none;
  found->second.health.detail = "restart acknowledged";
  return Result<bool>::success(true);
}
PlatformHealthReport HealthSupervisor::evaluate(TimePoint now) {
  std::unique_lock lock(mutex_);
  PlatformHealthReport report;
  for (auto& [name, entry] : entries_) {
    (void)name;
    if (now - entry.last_heartbeat > entry.policy.heartbeat_timeout) {
      entry.health.state = ComponentState::failed;
      if (entry.health.restart_count < entry.policy.restart_limit) entry.health.action = RecoveryAction::restart_component;
      else if (entry.policy.criticality == Criticality::quality_management) entry.health.action = RecoveryAction::enter_degraded_mode;
      else { entry.health.action = RecoveryAction::request_safe_state; report.safe_state_requested = true; }
      entry.health.detail = "heartbeat deadline missed";
    }
    report.components.push_back(entry.health);
  }
  if (report.safe_state_requested) report.aggregate = ComponentState::failed;
  else if (std::any_of(report.components.begin(), report.components.end(), [](const ComponentHealth& h) { return h.state != ComponentState::healthy; })) report.aggregate = ComponentState::degraded;
  return report;
}
}  // namespace smartvehicle
