#include "smartvehicle/ota/ab_slot.hpp"
#include "smartvehicle/platform/runtime.hpp"

#include <filesystem>
#include <iostream>

using namespace smartvehicle;
namespace {
UpdateManifest manifest() {
  return {"update-001", "vehicle-services", "1.1.0", "linux-x86_64",
          "artifact-v1.1", "test-hash:artifact-v1.1",
          "test-signature:artifact-v1.1", "test-root", true};
}
const char* slot_name(Slot slot) { return slot == Slot::a ? "A" : "B"; }
}  // namespace

int main() {
  auto crypto = std::make_shared<DeterministicTestCrypto>();
  UpdateManager update(crypto, "linux-x86_64", "1.0.0", {});
  update.offer(manifest());
  const auto installed = update.execute();
  std::cout << "update-state=" << (installed ? "verified" : installed.detail) << '\n';

  const auto journal = std::filesystem::temp_directory_path() / "smartvehicle-ota-demo.journal";
  std::filesystem::remove(journal);
  auto store = std::make_shared<FileSlotStore>(journal);
  AbUpdateCoordinator slots(store);
  slots.initialize("1.0.0");
  slots.stage("1.1.0");
  slots.request_activation();
  std::cout << "activation-slot=" << slot_name(slots.state().active)
            << " committed=false [SIMULATED A/B]\n";
  slots.report_boot_health(true);
  std::cout << "health=pass committed=" << std::boolalpha << slots.state().committed
            << " version=" << slots.state().active_version << '\n';

  slots.stage("1.2.0");
  slots.request_activation();
  slots.report_boot_health(false);
  std::cout << "health=fail rollback-slot=" << slot_name(slots.state().active)
            << " [SIMULATED BOOT DECISION]\n";

  slots.stage("1.3.0");
  slots.request_activation();
  AbUpdateCoordinator after_power_loss(store);
  after_power_loss.initialize("ignored");
  after_power_loss.recover_after_interruption();
  std::cout << "power-loss-recovery-slot=" << slot_name(after_power_loss.state().active)
            << " committed=" << after_power_loss.state().committed << '\n';
  std::filesystem::remove(journal);
}
