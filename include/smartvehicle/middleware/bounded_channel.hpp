#pragma once
#include "smartvehicle/platform/api.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <optional>
#include <span>
#include <string_view>

namespace smartvehicle {
constexpr std::size_t vehicle_message_capacity = 256;

struct VehicleMessage {
  std::uint32_t service_id{0};
  std::uint32_t instance_id{0};
  std::uint64_t sequence{0};
  std::uint8_t priority{0};
  std::size_t payload_size{0};
  std::array<std::byte, vehicle_message_capacity> payload{};

  static Result<VehicleMessage> create(std::uint32_t service, std::uint32_t instance,
                                       std::uint64_t sequence_number, std::uint8_t priority_value,
                                       std::span<const std::byte> bytes) {
    if (service == 0 || bytes.size() > vehicle_message_capacity) {
      return Result<VehicleMessage>::failure(ErrorCode::invalid_argument,
                                             "service id required and payload must be <= 256 bytes");
    }
    VehicleMessage message;
    message.service_id = service;
    message.instance_id = instance;
    message.sequence = sequence_number;
    message.priority = priority_value;
    message.payload_size = bytes.size();
    for (std::size_t i = 0; i < bytes.size(); ++i) message.payload[i] = bytes[i];
    return Result<VehicleMessage>::success(message);
  }

  [[nodiscard]] std::span<const std::byte> bytes() const noexcept {
    return {payload.data(), payload_size};
  }
};

template <class T, std::size_t Capacity>
class BoundedChannel {
  static_assert(Capacity > 0);
 public:
  Result<bool> try_push(T value) {
    std::lock_guard lock(mutex_);
    if (size_ == Capacity) return Result<bool>::failure(ErrorCode::queue_full, "channel full");
    slots_[tail_] = std::move(value);
    tail_ = (tail_ + 1) % Capacity;
    ++size_;
    high_watermark_ = size_ > high_watermark_ ? size_ : high_watermark_;
    return Result<bool>::success(true);
  }

  Result<T> try_pop() {
    std::lock_guard lock(mutex_);
    if (size_ == 0) return Result<T>::failure(ErrorCode::not_found, "channel empty");
    T value = std::move(*slots_[head_]);
    slots_[head_].reset();
    head_ = (head_ + 1) % Capacity;
    --size_;
    return Result<T>::success(std::move(value));
  }

  [[nodiscard]] std::size_t size() const { std::lock_guard lock(mutex_); return size_; }
  [[nodiscard]] std::size_t capacity() const noexcept { return Capacity; }
  [[nodiscard]] std::size_t high_watermark() const { std::lock_guard lock(mutex_); return high_watermark_; }

 private:
  mutable std::mutex mutex_;
  std::array<std::optional<T>, Capacity> slots_{};
  std::size_t head_{0};
  std::size_t tail_{0};
  std::size_t size_{0};
  std::size_t high_watermark_{0};
};
}  // namespace smartvehicle
