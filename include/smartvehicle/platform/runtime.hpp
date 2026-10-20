#pragma once
#include "smartvehicle/hal/vehicle.hpp"
#include "smartvehicle/middleware/in_process.hpp"
#include "smartvehicle/ota/update_manager.hpp"
#include <filesystem>
namespace smartvehicle {
struct PlatformConfig { std::string profile,transport,signal_provider,platform; std::vector<std::string> enabled_services; bool runnable{true}; };
Result<PlatformConfig> load_config(const std::filesystem::path&);
class AllowListAuthorizer final:public IAuthorizer { public: explicit AllowListAuthorizer(std::map<std::string,std::vector<std::string>>); bool allowed(std::string_view,std::string_view) const override; private: std::map<std::string,std::vector<std::string>,std::less<>> rules_; };
class DeterministicTestCrypto final:public ICryptoProvider { public: bool verify_hash(std::string_view,std::string_view) const override; bool verify_signature(std::string_view,std::string_view,std::string_view) const override; std::string random_id() override; };
}
