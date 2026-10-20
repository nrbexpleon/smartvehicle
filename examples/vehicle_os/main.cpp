#include "smartvehicle/middleware/bounded_channel.hpp"
#include "smartvehicle/platform/capabilities.hpp"
#include "smartvehicle/runtime/executor.hpp"
#include "smartvehicle/runtime/supervisor.hpp"
#include <array>
#include <chrono>
#include <iostream>

using namespace smartvehicle;
int main() {
  const auto host = detect_host_capabilities();
  const WorkloadRequirements requirements{1, 16U * 1024U * 1024U, true, false, false, false, true};
  const auto compatibility = evaluate_capabilities(host, requirements);
  std::cout << "platform=" << to_string(host.os) << '-' << to_string(host.architecture)
            << " cores=" << host.logical_cores << " compatible=" << std::boolalpha
            << compatibility.compatible << '\n';
  if (!compatibility.compatible) return 2;

  BoundedChannel<VehicleMessage, 8> channel;
  const std::array payload{std::byte{0x2a}, std::byte{0x01}};
  auto message = VehicleMessage::create(0x1001, 1, 1, 5, payload);
  if (!message || !channel.try_push(*message.value)) return 3;

  const auto epoch = DeterministicExecutor::TimePoint{};
  DeterministicExecutor executor(4);
  executor.register_periodic({"vehicle-state", std::chrono::milliseconds(10),
                              std::chrono::microseconds(500), Criticality::asil_b,
                              [&channel] {
                                const auto received = channel.try_pop();
                                return TaskExecution{std::chrono::microseconds(80), static_cast<bool>(received)};
                              }}, epoch);
  const auto dispatch = executor.run_ready(epoch, 4);

  HealthSupervisor supervisor;
  supervisor.register_component("vehicle-state", {std::chrono::milliseconds(50), 2, Criticality::asil_b}, epoch);
  supervisor.heartbeat("vehicle-state", !dispatch.safety_task_unhealthy, "cycle complete", epoch);
  const auto health = supervisor.evaluate(epoch + std::chrono::milliseconds(10));
  std::cout << "dispatches=" << dispatch.dispatched << " overruns=" << dispatch.budget_overruns
            << " channel-high-watermark=" << channel.high_watermark()
            << " safe-state=" << health.safe_state_requested << '\n';
  return health.safe_state_requested ? 4 : 0;
}
