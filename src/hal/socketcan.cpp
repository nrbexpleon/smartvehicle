#include "smartvehicle/hal/socketcan.hpp"

#include <cerrno>
#include <cstring>
#include <string>

#ifdef __linux__
#include <linux/can.h>
#include <linux/can/raw.h>
#include <net/if.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace smartvehicle {
LinuxSocketCan::~LinuxSocketCan() { close(); }

Result<bool> LinuxSocketCan::open(std::string_view interface_name) {
  close();
  if (interface_name.empty() || interface_name.size() >= IFNAMSIZ) {
    return Result<bool>::failure(ErrorCode::invalid_argument, "invalid CAN interface name");
  }
#ifdef __linux__
  socket_ = ::socket(PF_CAN, SOCK_RAW | SOCK_CLOEXEC, CAN_RAW);
  if (socket_ < 0) {
    return Result<bool>::failure(ErrorCode::invalid_state,
                                 "SocketCAN socket creation failed: " +
                                     std::string(std::strerror(errno)));
  }
  ifreq request{};
  std::memcpy(request.ifr_name, interface_name.data(), interface_name.size());
  if (::ioctl(socket_, SIOCGIFINDEX, &request) < 0) {
    const auto detail = std::string("SocketCAN interface lookup failed: ") +
                        std::strerror(errno);
    close();
    return Result<bool>::failure(ErrorCode::not_found, detail);
  }
  sockaddr_can address{};
  address.can_family = AF_CAN;
  address.can_ifindex = request.ifr_ifindex;
  if (::bind(socket_, reinterpret_cast<sockaddr*>(&address), sizeof(address)) < 0) {
    const auto detail = std::string("SocketCAN bind failed: ") + std::strerror(errno);
    close();
    return Result<bool>::failure(ErrorCode::invalid_state, detail);
  }
  return Result<bool>::success(true);
#else
  (void)interface_name;
  return Result<bool>::failure(ErrorCode::invalid_state,
                               "SocketCAN is available only on Linux");
#endif
}

Result<bool> LinuxSocketCan::send(const CanFrame& input) {
  if (input.payload.size() > 8) {
    return Result<bool>::failure(ErrorCode::invalid_argument,
                                 "classic CAN payload exceeds 8 bytes");
  }
  if (socket_ < 0) {
    return Result<bool>::failure(ErrorCode::invalid_state, "SocketCAN channel is not open");
  }
#ifdef __linux__
  can_frame frame{};
  frame.can_id = input.id;
  if (input.extended) frame.can_id |= CAN_EFF_FLAG;
  if (input.remote_request) frame.can_id |= CAN_RTR_FLAG;
  frame.can_dlc = static_cast<__u8>(input.payload.size());
  std::copy(input.payload.begin(), input.payload.end(), frame.data);
  if (::write(socket_, &frame, sizeof(frame)) != static_cast<ssize_t>(sizeof(frame))) {
    return Result<bool>::failure(ErrorCode::invalid_state,
                                 "SocketCAN write failed: " +
                                     std::string(std::strerror(errno)));
  }
  return Result<bool>::success(true);
#else
  return Result<bool>::failure(ErrorCode::invalid_state, "SocketCAN unsupported");
#endif
}

Result<CanFrame> LinuxSocketCan::receive(std::chrono::milliseconds timeout) {
  if (socket_ < 0) {
    return Result<CanFrame>::failure(ErrorCode::invalid_state,
                                     "SocketCAN channel is not open");
  }
  if (timeout < std::chrono::milliseconds::zero()) {
    return Result<CanFrame>::failure(ErrorCode::invalid_argument, "negative timeout");
  }
#ifdef __linux__
  pollfd descriptor{socket_, POLLIN, 0};
  const int ready = ::poll(&descriptor, 1, static_cast<int>(timeout.count()));
  if (ready == 0) return Result<CanFrame>::failure(ErrorCode::timeout, "CAN receive timeout");
  if (ready < 0) {
    return Result<CanFrame>::failure(ErrorCode::invalid_state,
                                     "SocketCAN poll failed: " +
                                         std::string(std::strerror(errno)));
  }
  can_frame frame{};
  if (::read(socket_, &frame, sizeof(frame)) != static_cast<ssize_t>(sizeof(frame))) {
    return Result<CanFrame>::failure(ErrorCode::invalid_state, "invalid SocketCAN frame");
  }
  CanFrame output;
  output.extended = (frame.can_id & CAN_EFF_FLAG) != 0;
  output.remote_request = (frame.can_id & CAN_RTR_FLAG) != 0;
  output.id = frame.can_id & (output.extended ? CAN_EFF_MASK : CAN_SFF_MASK);
  output.payload.assign(frame.data, frame.data + frame.can_dlc);
  return Result<CanFrame>::success(std::move(output));
#else
  (void)timeout;
  return Result<CanFrame>::failure(ErrorCode::invalid_state, "SocketCAN unsupported");
#endif
}

void LinuxSocketCan::close() noexcept {
#ifdef __linux__
  if (socket_ >= 0) ::close(socket_);
#endif
  socket_ = -1;
}

bool LinuxSocketCan::is_open() const noexcept { return socket_ >= 0; }
}  // namespace smartvehicle
