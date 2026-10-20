#pragma once
#include <chrono>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
namespace smartvehicle {
struct ApiVersion { std::uint16_t major{1}; std::uint16_t minor{0}; [[nodiscard]] bool satisfies(const ApiVersion& r) const noexcept { return major==r.major && minor>=r.minor; } auto operator<=>(const ApiVersion&) const = default; };
enum class ErrorCode { ok, invalid_argument, not_found, already_exists, incompatible_version, timeout, queue_full, unauthorized, invalid_state, integrity_failure, signature_failure, incompatible_artifact, insufficient_storage, interrupted, health_check_failed, rollback_failed };
template<class T> struct Result { std::optional<T> value; ErrorCode error{ErrorCode::ok}; std::string detail; [[nodiscard]] explicit operator bool() const noexcept { return value.has_value(); } static Result success(T v){ return {std::move(v),ErrorCode::ok,{}}; } static Result failure(ErrorCode e,std::string d){ return {std::nullopt,e,std::move(d)}; } };
class IClock { public: virtual ~IClock()=default; virtual std::chrono::steady_clock::time_point monotonic_now() const=0; virtual std::chrono::system_clock::time_point utc_now() const=0; };
class ILogger { public: virtual ~ILogger()=default; virtual void log(std::string_view,std::string_view,std::string_view)=0; };
class IAuthorizer { public: virtual ~IAuthorizer()=default; virtual bool allowed(std::string_view principal,std::string_view permission) const=0; };
class ICryptoProvider { public: virtual ~ICryptoProvider()=default; virtual bool verify_hash(std::string_view payload,std::string_view expected) const=0; virtual bool verify_signature(std::string_view payload,std::string_view signature,std::string_view trust_anchor) const=0; virtual std::string random_id()=0; };
}
