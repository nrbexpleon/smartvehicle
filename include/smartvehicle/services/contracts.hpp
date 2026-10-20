#pragma once
#include "smartvehicle/platform/api.hpp"
#include <functional>
#include <map>
#include <mutex>
#include <shared_mutex>
#include <variant>
#include <vector>
namespace smartvehicle {
using SignalValue=std::variant<bool,std::int64_t,double,std::string>;
enum class SignalValidity { valid, stale, invalid, unavailable };
struct VehicleSignal { std::string id; SignalValue value; std::string unit; std::chrono::system_clock::time_point timestamp; SignalValidity validity{SignalValidity::valid}; std::string source; double quality{1.0}; std::uint64_t sequence{0}; };
enum class HealthStatus { healthy, degraded, failed, stopped };
struct ServiceHealth { HealthStatus status{HealthStatus::stopped}; std::string detail; std::uint64_t error_count{0}; };
struct ServiceDescriptor { std::string id; ApiVersion version; std::string implementation; std::vector<std::string> permissions; };
class IServiceRegistry { public: virtual ~IServiceRegistry()=default; virtual Result<bool> register_service(ServiceDescriptor)=0; virtual Result<ServiceDescriptor> discover(std::string_view,ApiVersion) const=0; };
class IEventBus { public: using Handler=std::function<void(const VehicleSignal&)>; virtual ~IEventBus()=default; virtual Result<std::uint64_t> subscribe(std::string_view,Handler,std::size_t queue_limit=64)=0; virtual Result<bool> publish(std::string_view,const VehicleSignal&,std::chrono::milliseconds)=0; virtual void unsubscribe(std::uint64_t)=0; };
class IVehicleSignalProvider { public: static constexpr std::string_view interface_id="org.smartvehicle.signal-provider"; static constexpr ApiVersion version{1,0}; virtual ~IVehicleSignalProvider()=default; virtual Result<VehicleSignal> read(std::string_view,std::chrono::milliseconds)=0; virtual std::string_view name() const noexcept=0; };
class IVehicleSignalService { public: static constexpr std::string_view service_id="org.smartvehicle.vehicle-signals"; static constexpr ApiVersion version{1,0}; virtual ~IVehicleSignalService()=default; virtual Result<VehicleSignal> get(std::string_view,std::string_view,std::chrono::milliseconds)=0; virtual Result<bool> sample_and_publish(std::string_view)=0; virtual ServiceHealth health() const=0; };
class VehicleSignalService final:public IVehicleSignalService { public: VehicleSignalService(std::shared_ptr<IVehicleSignalProvider>,std::shared_ptr<IEventBus>,std::shared_ptr<IAuthorizer>); Result<VehicleSignal> get(std::string_view,std::string_view,std::chrono::milliseconds) override; Result<bool> sample_and_publish(std::string_view) override; ServiceHealth health() const override; void replace_provider(std::shared_ptr<IVehicleSignalProvider>); private: mutable std::mutex mutex_; std::shared_ptr<IVehicleSignalProvider> provider_; std::shared_ptr<IEventBus> events_; std::shared_ptr<IAuthorizer> auth_; ServiceHealth health_{HealthStatus::healthy,"ready",0}; };
}
