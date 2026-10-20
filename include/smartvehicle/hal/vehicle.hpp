#pragma once
#include "smartvehicle/services/contracts.hpp"
namespace smartvehicle {
class SimulatedSignalProvider final:public IVehicleSignalProvider { public: SimulatedSignalProvider(std::string,std::map<std::string,SignalValue>); Result<VehicleSignal> read(std::string_view,std::chrono::milliseconds) override; std::string_view name() const noexcept override; void set(std::string,SignalValue); private: std::string name_; std::map<std::string,SignalValue,std::less<>> values_; std::mutex mutex_; std::uint64_t sequence_{0}; };
class ICanBoundary { public: virtual ~ICanBoundary()=default; virtual Result<bool> open(std::string_view)=0; virtual Result<bool> send(std::uint32_t,const std::vector<std::uint8_t>&)=0; };
class DisabledActuatorGate { public: Result<bool> command(std::string_view) const { return Result<bool>::failure(ErrorCode::unauthorized,"physical actuator commands disabled in host reference"); } };
}
