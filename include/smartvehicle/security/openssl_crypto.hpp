#pragma once

#include "smartvehicle/platform/api.hpp"

namespace smartvehicle {

// Host/embedded-Linux crypto adapter backed by OpenSSL EVP. Trust anchors are
// supplied as PEM public keys; private keys never enter this interface.
class OpenSslCryptoProvider final : public ICryptoProvider {
 public:
  [[nodiscard]] bool verify_hash(std::string_view payload,
                                 std::string_view expected) const override;
  [[nodiscard]] bool verify_signature(std::string_view payload,
                                      std::string_view signature,
                                      std::string_view trust_anchor) const override;
  [[nodiscard]] std::string random_id() override;
};

}  // namespace smartvehicle
