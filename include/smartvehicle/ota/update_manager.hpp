#pragma once
#include "smartvehicle/platform/api.hpp"
#include <vector>
namespace smartvehicle {
enum class UpdateState { idle,available,validating_manifest,downloading,verifying_integrity,verifying_signature,checking_compatibility,staging,preparing_installation,installing,activation_requested,verifying_health,committed,rolling_back,failed };
struct UpdateManifest { std::string update_id,component,version,target_platform,artifact,artifact_hash,signature,trust_anchor; bool rollback_eligible{true}; };
struct UpdatePolicy { bool allow_downgrade{false}; std::size_t available_storage_bytes{1024*1024}; };
struct AuditEvent { UpdateState from,to; ErrorCode outcome; std::string detail; };
class UpdateManager { public: UpdateManager(std::shared_ptr<ICryptoProvider>,std::string platform,std::string current_version,UpdatePolicy); Result<UpdateState> offer(UpdateManifest); Result<UpdateState> execute(bool interrupt_download=false,bool fail_health=false); UpdateState state() const noexcept; const std::vector<AuditEvent>& audit_log() const noexcept; private: Result<UpdateState> transition(UpdateState,std::string); Result<UpdateState> fail(ErrorCode,std::string,bool rollback); std::shared_ptr<ICryptoProvider> crypto_; std::string platform_,current_version_; UpdatePolicy policy_; std::optional<UpdateManifest> manifest_; UpdateState state_{UpdateState::idle}; std::vector<AuditEvent> audit_; };
}
