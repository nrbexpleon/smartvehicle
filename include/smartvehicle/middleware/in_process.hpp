#pragma once
#include "smartvehicle/services/contracts.hpp"
#include <atomic>
namespace smartvehicle {
class InProcessServiceRegistry final:public IServiceRegistry { public: Result<bool> register_service(ServiceDescriptor) override; Result<ServiceDescriptor> discover(std::string_view,ApiVersion) const override; private: mutable std::shared_mutex mutex_; std::map<std::string,ServiceDescriptor,std::less<>> services_; };
class InProcessEventBus final:public IEventBus { public: Result<std::uint64_t> subscribe(std::string_view,Handler,std::size_t) override; Result<bool> publish(std::string_view,const VehicleSignal&,std::chrono::milliseconds) override; void unsubscribe(std::uint64_t) override; private: struct Sub{std::string topic;Handler handler;std::size_t limit;}; std::mutex mutex_; std::map<std::uint64_t,Sub> subs_; std::atomic_uint64_t next_{1}; };
}
