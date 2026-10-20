#pragma once
#include "smartvehicle/platform/api.hpp"
#include <chrono>
#include <cstddef>
#include <functional>
#include <string>
#include <vector>

namespace smartvehicle {
enum class Criticality { quality_management, asil_a, asil_b, asil_c, asil_d };

struct TaskExecution {
  std::chrono::nanoseconds consumed{0};
  bool healthy{true};
};

struct TaskSpec {
  std::string name;
  std::chrono::milliseconds period{100};
  std::chrono::microseconds budget{1000};
  Criticality criticality{Criticality::quality_management};
  std::function<TaskExecution()> callback;
};

struct TaskStatistics {
  std::uint64_t dispatches{0};
  std::uint64_t budget_overruns{0};
  std::uint64_t unhealthy_runs{0};
  std::chrono::nanoseconds maximum_consumed{0};
};

struct DispatchSummary {
  std::size_t dispatched{0};
  std::size_t deferred{0};
  std::size_t budget_overruns{0};
  bool safety_task_unhealthy{false};
};

class DeterministicExecutor {
 public:
  using TimePoint = std::chrono::steady_clock::time_point;
  explicit DeterministicExecutor(std::size_t maximum_tasks);
  Result<std::size_t> register_periodic(TaskSpec, TimePoint first_release);
  DispatchSummary run_ready(TimePoint now, std::size_t maximum_dispatches);
  [[nodiscard]] Result<TaskStatistics> statistics(std::string_view name) const;
  [[nodiscard]] std::size_t task_count() const noexcept;

 private:
  struct Task { TaskSpec spec; TimePoint next_release; TaskStatistics stats; std::size_t order; };
  std::size_t maximum_tasks_;
  std::vector<Task> tasks_;
};
}  // namespace smartvehicle
