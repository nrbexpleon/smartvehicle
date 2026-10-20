#include "smartvehicle/security/openssl_crypto.hpp"

#include <array>
#include <cctype>
#include <iomanip>
#include <memory>
#include <sstream>
#include <vector>

#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rand.h>
#include <openssl/sha.h>

namespace smartvehicle {
namespace {
std::string to_hex(const unsigned char* data, std::size_t size) {
  std::ostringstream output;
  output << std::hex << std::setfill('0');
  for (std::size_t index = 0; index < size; ++index) {
    output << std::setw(2) << static_cast<unsigned int>(data[index]);
  }
  return output.str();
}

std::optional<std::vector<unsigned char>> from_hex(std::string_view text) {
  if (text.size() % 2 != 0) return std::nullopt;
  std::vector<unsigned char> bytes;
  bytes.reserve(text.size() / 2);
  for (std::size_t index = 0; index < text.size(); index += 2) {
    if (!std::isxdigit(static_cast<unsigned char>(text[index])) ||
        !std::isxdigit(static_cast<unsigned char>(text[index + 1]))) {
      return std::nullopt;
    }
    const auto value = std::stoul(std::string(text.substr(index, 2)), nullptr, 16);
    bytes.push_back(static_cast<unsigned char>(value));
  }
  return bytes;
}
}  // namespace

bool OpenSslCryptoProvider::verify_hash(std::string_view payload,
                                        std::string_view expected) const {
  std::array<unsigned char, SHA256_DIGEST_LENGTH> digest{};
  if (EVP_Digest(payload.data(), payload.size(), digest.data(), nullptr, EVP_sha256(),
                 nullptr) != 1) {
    return false;
  }
  constexpr std::string_view prefix = "sha256:";
  if (expected.starts_with(prefix)) expected.remove_prefix(prefix.size());
  return to_hex(digest.data(), digest.size()) == expected;
}

bool OpenSslCryptoProvider::verify_signature(std::string_view payload,
                                             std::string_view signature,
                                             std::string_view trust_anchor) const {
  constexpr std::string_view prefix = "hex:";
  if (signature.starts_with(prefix)) signature.remove_prefix(prefix.size());
  const auto decoded = from_hex(signature);
  if (!decoded) return false;

  std::unique_ptr<BIO, decltype(&BIO_free)> bio(
      BIO_new_mem_buf(trust_anchor.data(), static_cast<int>(trust_anchor.size())), &BIO_free);
  if (!bio) return false;
  std::unique_ptr<EVP_PKEY, decltype(&EVP_PKEY_free)> key(
      PEM_read_bio_PUBKEY(bio.get(), nullptr, nullptr, nullptr), &EVP_PKEY_free);
  if (!key) return false;
  std::unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)> context(EVP_MD_CTX_new(),
                                                                  &EVP_MD_CTX_free);
  if (!context) return false;
  if (EVP_DigestVerifyInit(context.get(), nullptr, EVP_sha256(), nullptr, key.get()) != 1) {
    return false;
  }
  return EVP_DigestVerify(context.get(), decoded->data(), decoded->size(),
                          reinterpret_cast<const unsigned char*>(payload.data()),
                          payload.size()) == 1;
}

std::string OpenSslCryptoProvider::random_id() {
  std::array<unsigned char, 16> bytes{};
  if (RAND_bytes(bytes.data(), bytes.size()) != 1) return {};
  // RFC 4122 variant/version bits for a random UUID.
  bytes[6] = static_cast<unsigned char>((bytes[6] & 0x0F) | 0x40);
  bytes[8] = static_cast<unsigned char>((bytes[8] & 0x3F) | 0x80);
  const auto hex = to_hex(bytes.data(), bytes.size());
  return hex.substr(0, 8) + "-" + hex.substr(8, 4) + "-" + hex.substr(12, 4) + "-" +
         hex.substr(16, 4) + "-" + hex.substr(20, 12);
}

}  // namespace smartvehicle
