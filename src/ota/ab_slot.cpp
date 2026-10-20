#include "smartvehicle/ota/ab_slot.hpp"

#include <fstream>
#include <map>
#include <sstream>

namespace smartvehicle {
namespace {
char slot_name(Slot slot) { return slot == Slot::a ? 'A' : 'B'; }
std::optional<Slot> parse_slot(std::string_view value) {
  if (value == "A") return Slot::a;
  if (value == "B") return Slot::b;
  return std::nullopt;
}
}  // namespace

FileSlotStore::FileSlotStore(std::filesystem::path journal) : journal_(std::move(journal)) {}

Result<SlotState> FileSlotStore::load() const {
  std::ifstream input(journal_);
  if (!input) return Result<SlotState>::failure(ErrorCode::not_found, "slot journal not found");
  std::map<std::string, std::string> values;
  std::string line;
  while (std::getline(input, line)) {
    const auto separator = line.find('=');
    if (separator == std::string::npos) {
      return Result<SlotState>::failure(ErrorCode::invalid_argument, "malformed slot journal");
    }
    values[line.substr(0, separator)] = line.substr(separator + 1);
  }
  const auto active = parse_slot(values["active"]);
  const auto previous = parse_slot(values["previous"]);
  if (!active || !previous || !values.contains("active_version")) {
    return Result<SlotState>::failure(ErrorCode::invalid_argument, "incomplete slot journal");
  }
  SlotState state;
  state.active = *active;
  state.previous = *previous;
  if (values["pending"] != "-") state.pending = parse_slot(values["pending"]);
  state.active_version = values["active_version"];
  state.pending_version = values["pending_version"];
  try {
    state.failed_boots = static_cast<std::uint32_t>(std::stoul(values["failed_boots"]));
  } catch (...) {
    return Result<SlotState>::failure(ErrorCode::invalid_argument, "invalid boot counter");
  }
  state.committed = values["committed"] == "1";
  return Result<SlotState>::success(std::move(state));
}

Result<bool> FileSlotStore::save(const SlotState& state) {
  std::error_code error;
  if (!journal_.parent_path().empty()) {
    std::filesystem::create_directories(journal_.parent_path(), error);
    if (error) return Result<bool>::failure(ErrorCode::invalid_state, error.message());
  }
  auto temporary = journal_;
  temporary += ".tmp";
  {
    std::ofstream output(temporary, std::ios::trunc);
    if (!output) return Result<bool>::failure(ErrorCode::invalid_state, "cannot write slot journal");
    output << "active=" << slot_name(state.active) << '\n'
           << "previous=" << slot_name(state.previous) << '\n'
           << "pending=" << (state.pending ? std::string(1, slot_name(*state.pending)) : "-")
           << '\n'
           << "active_version=" << state.active_version << '\n'
           << "pending_version=" << state.pending_version << '\n'
           << "failed_boots=" << state.failed_boots << '\n'
           << "committed=" << (state.committed ? 1 : 0) << '\n';
    output.flush();
    if (!output) return Result<bool>::failure(ErrorCode::invalid_state, "slot journal flush failed");
  }
  std::filesystem::rename(temporary, journal_, error);
  if (error) {
    std::filesystem::remove(journal_, error);
    error.clear();
    std::filesystem::rename(temporary, journal_, error);
  }
  if (error) return Result<bool>::failure(ErrorCode::invalid_state, error.message());
  return Result<bool>::success(true);
}

AbUpdateCoordinator::AbUpdateCoordinator(std::shared_ptr<ISlotStore> store,
                                         std::uint32_t max_failed_boots)
    : store_(std::move(store)), max_failed_boots_(max_failed_boots) {}

Result<SlotState> AbUpdateCoordinator::persist() {
  auto saved = store_->save(state_);
  if (!saved) return Result<SlotState>::failure(saved.error, saved.detail);
  return Result<SlotState>::success(state_);
}

Result<SlotState> AbUpdateCoordinator::initialize(std::string current_version) {
  auto loaded = store_->load();
  if (loaded) {
    state_ = *loaded.value;
  } else if (loaded.error == ErrorCode::not_found) {
    state_ = {};
    state_.active_version = std::move(current_version);
    auto saved = persist();
    if (!saved) return saved;
  } else {
    return loaded;
  }
  loaded_ = true;
  return Result<SlotState>::success(state_);
}

Result<SlotState> AbUpdateCoordinator::stage(std::string version) {
  if (!loaded_ || version.empty()) {
    return Result<SlotState>::failure(ErrorCode::invalid_state,
                                      "slot coordinator not initialized or version empty");
  }
  state_.pending = state_.active == Slot::a ? Slot::b : Slot::a;
  state_.pending_version = std::move(version);
  state_.previous = state_.active;
  state_.failed_boots = 0;
  state_.committed = false;
  return persist();
}

Result<SlotState> AbUpdateCoordinator::request_activation() {
  if (!loaded_ || !state_.pending) {
    return Result<SlotState>::failure(ErrorCode::invalid_state, "no staged slot");
  }
  state_.active = *state_.pending;
  return persist();
}

Result<SlotState> AbUpdateCoordinator::report_boot_health(bool healthy) {
  if (!loaded_ || state_.committed || !state_.pending) {
    return Result<SlotState>::failure(ErrorCode::invalid_state, "no activation awaiting health");
  }
  if (healthy) {
    state_.active_version = state_.pending_version;
    state_.pending.reset();
    state_.pending_version.clear();
    state_.failed_boots = 0;
    state_.committed = true;
    return persist();
  }
  ++state_.failed_boots;
  if (state_.failed_boots >= max_failed_boots_) {
    state_.active = state_.previous;
    state_.pending.reset();
    state_.pending_version.clear();
    state_.committed = true;
  }
  return persist();
}

Result<SlotState> AbUpdateCoordinator::recover_after_interruption() {
  if (!loaded_) {
    return Result<SlotState>::failure(ErrorCode::invalid_state, "coordinator not initialized");
  }
  if (!state_.committed && state_.pending && state_.active == *state_.pending) {
    state_.active = state_.previous;
    state_.pending.reset();
    state_.pending_version.clear();
    state_.committed = true;
    ++state_.failed_boots;
    return persist();
  }
  return Result<SlotState>::success(state_);
}

const SlotState& AbUpdateCoordinator::state() const noexcept { return state_; }
}  // namespace smartvehicle
