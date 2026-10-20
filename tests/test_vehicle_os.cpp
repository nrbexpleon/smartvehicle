#include "smartvehicle/middleware/bounded_channel.hpp"
#include "smartvehicle/platform/capabilities.hpp"
#include "smartvehicle/runtime/executor.hpp"
#include "smartvehicle/runtime/supervisor.hpp"
#include <array>
#include <chrono>
#include <iostream>
#include <stdexcept>

using namespace smartvehicle;
namespace {
int passed = 0;
#define CHECK(name, expression) do { if (!(expression)) throw std::runtime_error(name); ++passed; } while (false)
}

int main() {
  try {
    PlatformCapabilities platform{OsFamily::linux, CpuArchitecture::arm64, 4, 128U*1024U*1024U, true, true, true, true, true, true};
    WorkloadRequirements workload{2, 64U*1024U*1024U, true, false, true, true, true};
    CHECK("compatible platform", evaluate_capabilities(platform, workload).compatible);
    workload.minimum_cores = 8;
    auto incompatible = evaluate_capabilities(platform, workload);
    CHECK("incompatible core count", !incompatible.compatible);
    CHECK("unmet reason", incompatible.unmet.front() == "logical_cores");
    CHECK("os name", to_string(OsFamily::zephyr) == "zephyr");
    CHECK("architecture name", to_string(CpuArchitecture::arm64) == "arm64");

    BoundedChannel<int, 2> numbers;
    CHECK("queue first", numbers.try_push(1));
    CHECK("queue second", numbers.try_push(2));
    CHECK("queue bounded", numbers.try_push(3).error == ErrorCode::queue_full);
    CHECK("queue size", numbers.size() == 2);
    CHECK("queue watermark", numbers.high_watermark() == 2);
    CHECK("queue order one", numbers.try_pop().value == 1);
    CHECK("queue order two", numbers.try_pop().value == 2);
    CHECK("queue empty", numbers.try_pop().error == ErrorCode::not_found);

    std::array<std::byte, 257> excessive{};
    CHECK("message payload bounded", !VehicleMessage::create(1, 1, 1, 0, excessive));
    std::array<std::byte, 2> bytes{std::byte{1}, std::byte{2}};
    auto message = VehicleMessage::create(1, 2, 3, 4, bytes);
    CHECK("message created", message);
    CHECK("message length", message.value->bytes().size() == 2);

    const auto epoch = DeterministicExecutor::TimePoint{};
    DeterministicExecutor executor(2);
    int safety_runs = 0;
    CHECK("register quality", executor.register_periodic({"quality", std::chrono::milliseconds(20), std::chrono::microseconds(100), Criticality::quality_management, [] { return TaskExecution{std::chrono::microseconds(150), true}; }}, epoch));
    CHECK("register safety", executor.register_periodic({"safety", std::chrono::milliseconds(10), std::chrono::microseconds(100), Criticality::asil_d, [&] { ++safety_runs; return TaskExecution{std::chrono::microseconds(50), true}; }}, epoch));
    CHECK("executor bounded", executor.register_periodic({"third", std::chrono::milliseconds(10), std::chrono::microseconds(1), Criticality::asil_a, [] { return TaskExecution{}; }}, epoch).error == ErrorCode::queue_full);
    auto dispatch = executor.run_ready(epoch, 1);
    CHECK("one dispatch", dispatch.dispatched == 1 && dispatch.deferred == 1);
    CHECK("criticality first", safety_runs == 1);
    dispatch = executor.run_ready(epoch, 2);
    CHECK("remaining quality", dispatch.dispatched == 1 && dispatch.budget_overruns == 1);
    CHECK("quality statistics", executor.statistics("quality").value->budget_overruns == 1);
    CHECK("unknown statistics", executor.statistics("missing").error == ErrorCode::not_found);

    HealthSupervisor supervisor;
    CHECK("register supervisor", supervisor.register_component("brakes", {std::chrono::milliseconds(10), 1, Criticality::asil_d}, epoch));
    CHECK("duplicate component", supervisor.register_component("brakes", {}, epoch).error == ErrorCode::already_exists);
    CHECK("heartbeat", supervisor.heartbeat("brakes", true, "nominal", epoch));
    CHECK("healthy aggregate", supervisor.evaluate(epoch + std::chrono::milliseconds(5)).aggregate == ComponentState::healthy);
    auto missed = supervisor.evaluate(epoch + std::chrono::milliseconds(11));
    CHECK("restart requested", missed.components.front().action == RecoveryAction::restart_component);
    CHECK("restart acknowledged", supervisor.acknowledge_restart("brakes", epoch + std::chrono::milliseconds(11)));
    missed = supervisor.evaluate(epoch + std::chrono::milliseconds(22));
    CHECK("safe state requested", missed.safe_state_requested);
    CHECK("failed aggregate", missed.aggregate == ComponentState::failed);
    CHECK("unknown heartbeat", supervisor.heartbeat("missing", true, "", epoch).error == ErrorCode::not_found);

    std::cout << "PASS " << passed << " vehicle OS checks\n";
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "FAIL after " << passed << " checks: " << error.what() << '\n';
    return 1;
  }
}
