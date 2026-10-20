#pragma once

#include "smartvehicle/platform/api.hpp"

#include <filesystem>

namespace smartvehicle {

enum class Slot { a, b };

struct SlotState {
  Slot active{Slot::a};
  Slot previous{Slot::a};
  std::optional<Slot> pending;
  std::string active_version{"unknown"};
  std::string pending_version;
  std::uint32_t failed_boots{0};
  bool committed{true};
};

class ISlotStore {
 public:
  virtual ~ISlotStore() = default;
  virtual Result<SlotState> load() const = 0;
  virtual Result<bool> save(const SlotState& state) = 0;
};

// Atomic filesystem journal for the host and embedded-Linux reference. It
// models boot decisions but does not claim to replace a bootloader.
class FileSlotStore final : public ISlotStore {
 public:
  explicit FileSlotStore(std::filesystem::path journal);
  Result<SlotState> load() const override;
  Result<bool> save(const SlotState& state) override;

 private:
  std::filesystem::path journal_;
};

class AbUpdateCoordinator {
 public:
  AbUpdateCoordinator(std::shared_ptr<ISlotStore> store, std::uint32_t max_failed_boots = 1);
  Result<SlotState> initialize(std::string current_version);
  Result<SlotState> stage(std::string version);
  Result<SlotState> request_activation();
  Result<SlotState> report_boot_health(bool healthy);
  Result<SlotState> recover_after_interruption();
  [[nodiscard]] const SlotState& state() const noexcept;

 private:
  Result<SlotState> persist();
  std::shared_ptr<ISlotStore> store_;
  SlotState state_;
  std::uint32_t max_failed_boots_;
  bool loaded_{false};
};

}  // namespace smartvehicle
