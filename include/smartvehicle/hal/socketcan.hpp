#pragma once

#include "smartvehicle/platform/api.hpp"

#include <chrono>
#include <cstdint>
#include <span>
#include <vector>

namespace smartvehicle {

struct CanFrame {
  std::uint32_t id{0};
  bool extended{false};
  bool remote_request{false};
  std::vector<std::uint8_t> payload;
};

class ICanChannel {
 public:
  virtual ~ICanChannel() = default;
  virtual Result<bool> open(std::string_view interface_name) = 0;
  virtual Result<bool> send(const CanFrame& frame) = 0;
  virtual Result<CanFrame> receive(std::chrono::milliseconds timeout) = 0;
  virtual void close() noexcept = 0;
};

// Concrete Linux SocketCAN backend. It can use physical CAN interfaces or a
// vcan interface. The public header deliberately exposes no Linux SDK types.
class LinuxSocketCan final : public ICanChannel {
 public:
  LinuxSocketCan() = default;
  ~LinuxSocketCan() override;
  LinuxSocketCan(const LinuxSocketCan&) = delete;
  LinuxSocketCan& operator=(const LinuxSocketCan&) = delete;

  Result<bool> open(std::string_view interface_name) override;
  Result<bool> send(const CanFrame& frame) override;
  Result<CanFrame> receive(std::chrono::milliseconds timeout) override;
  void close() noexcept override;
  [[nodiscard]] bool is_open() const noexcept;

 private:
  int socket_{-1};
};

}  // namespace smartvehicle
