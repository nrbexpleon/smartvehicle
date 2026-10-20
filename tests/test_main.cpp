#include "smartvehicle/hal/socketcan.hpp"
#include "smartvehicle/ota/ab_slot.hpp"
#include "smartvehicle/platform/runtime.hpp"
#ifdef SMARTVEHICLE_HAS_OPENSSL
#include "smartvehicle/security/openssl_crypto.hpp"
#endif

#include <filesystem>
#include <iostream>
#include <stdexcept>

using namespace smartvehicle;
namespace {
int passed = 0;
#define CHECK(name, expression)             \
  do {                                      \
    if (!(expression)) throw std::runtime_error(name); \
    ++passed;                               \
  } while (false)

UpdateManifest manifest() {
  return {"u1", "core", "1.1.0", "linux-x86_64", "payload", "test-hash:payload",
          "test-signature:payload", "test-root", true};
}
}  // namespace

int main() {
  try {
    CHECK("version", (ApiVersion{1, 2}.satisfies({1, 1})));
    CHECK("major", (!ApiVersion{2, 0}.satisfies({1, 0})));

    InProcessServiceRegistry registry;
    CHECK("register", registry.register_service({"svc", {1, 2}, "impl", {}}));
    CHECK("duplicate", registry.register_service({"svc", {1, 2}, "impl", {}}).error ==
                           ErrorCode::already_exists);
    CHECK("discover", registry.discover("svc", {1, 1}));
    CHECK("incompatible", registry.discover("svc", {1, 3}).error ==
                              ErrorCode::incompatible_version);

    auto bus = std::make_shared<InProcessEventBus>();
    int delivered = 0;
    auto token = bus->subscribe("vehicle.signals",
                                [&](const VehicleSignal&) { ++delivered; }, 64);
    VehicleSignal event{"Vehicle.Speed", 1.0, "km/h", {}, SignalValidity::valid, "test",
                        1.0, 1};
    CHECK("publish", bus->publish("vehicle.signals", event, std::chrono::milliseconds(1)));
    CHECK("delivery", delivered == 1);
    CHECK("timeout", bus->publish("vehicle.signals", event, std::chrono::milliseconds(0)).error ==
                         ErrorCode::timeout);
    bus->unsubscribe(*token.value);

    auto auth = std::make_shared<AllowListAuthorizer>(
        std::map<std::string, std::vector<std::string>>{{"app", {"signals.read"}}});
    auto provider_a = std::make_shared<SimulatedSignalProvider>(
        "a", std::map<std::string, SignalValue>{{"Vehicle.Speed", 3.0}});
    VehicleSignalService service(provider_a, bus, auth);
    CHECK("authorized", service.get("app", "Vehicle.Speed", std::chrono::milliseconds(1)));
    CHECK("unauthorized", service.get("attacker", "Vehicle.Speed",
                                      std::chrono::milliseconds(1)).error ==
                              ErrorCode::unauthorized);
    CHECK("unknown", service.get("app", "Unknown", std::chrono::milliseconds(1)).error ==
                         ErrorCode::not_found);
    auto provider_b = std::make_shared<SimulatedSignalProvider>(
        "b", std::map<std::string, SignalValue>{{"Vehicle.Speed", 4.0}});
    service.replace_provider(provider_b);
    CHECK("substitution", service.get("app", "Vehicle.Speed", std::chrono::milliseconds(1))
                                      .value->source == "b");
    CHECK("actuator gate", DisabledActuatorGate{}.command("move").error ==
                               ErrorCode::unauthorized);

    auto test_crypto = std::make_shared<DeterministicTestCrypto>();
    UpdateManager success(test_crypto, "linux-x86_64", "1.0.0", {});
    CHECK("offer", success.offer(manifest()));
    CHECK("commit", success.execute().value == UpdateState::committed);
    auto bad = manifest();
    bad.artifact_hash = "wrong";
    UpdateManager hash(test_crypto, "linux-x86_64", "1.0.0", {});
    hash.offer(bad);
    CHECK("hash", hash.execute().error == ErrorCode::integrity_failure);
    bad = manifest();
    bad.signature = "wrong";
    UpdateManager signature(test_crypto, "linux-x86_64", "1.0.0", {});
    signature.offer(bad);
    CHECK("signature", signature.execute().error == ErrorCode::signature_failure);
    bad = manifest();
    bad.target_platform = "arm64";
    UpdateManager compatibility(test_crypto, "linux-x86_64", "1.0.0", {});
    compatibility.offer(bad);
    CHECK("target", compatibility.execute().error == ErrorCode::incompatible_artifact);
    UpdateManager interruption(test_crypto, "linux-x86_64", "1.0.0", {});
    interruption.offer(manifest());
    CHECK("interrupt", interruption.execute(true).error == ErrorCode::interrupted);
    UpdateManager rollback(test_crypto, "linux-x86_64", "1.0.0", {});
    rollback.offer(manifest());
    CHECK("health", rollback.execute(false, true).error == ErrorCode::health_check_failed);
    CHECK("rollback", rollback.audit_log().at(rollback.audit_log().size() - 2).to ==
                          UpdateState::rolling_back);

    LinuxSocketCan can;
    CHECK("can initially closed", !can.is_open());
    CHECK("can empty interface", can.open("").error == ErrorCode::invalid_argument);
    CHECK("can payload bound",
          can.send({0x123, false, false, std::vector<std::uint8_t>(9, 0)}).error ==
              ErrorCode::invalid_argument);
    CHECK("can unopened send",
          can.send({0x123, false, false, {1, 2, 3}}).error == ErrorCode::invalid_state);
    CHECK("can unopened receive",
          can.receive(std::chrono::milliseconds(1)).error == ErrorCode::invalid_state);

    const auto journal = std::filesystem::temp_directory_path() /
                         ("smartvehicle-slot-" + test_crypto->random_id() + ".journal");
    std::filesystem::remove(journal);
    auto store = std::make_shared<FileSlotStore>(journal);
    AbUpdateCoordinator slots(store);
    CHECK("slot initialize", slots.initialize("1.0.0"));
    CHECK("slot initial A", slots.state().active == Slot::a && slots.state().committed);
    CHECK("slot stage", slots.stage("1.1.0"));
    CHECK("slot pending B", slots.state().pending == Slot::b && !slots.state().committed);
    CHECK("slot activate", slots.request_activation());
    CHECK("slot active B", slots.state().active == Slot::b);
    CHECK("slot rollback unhealthy", slots.report_boot_health(false));
    CHECK("slot rolled back A", slots.state().active == Slot::a && slots.state().committed);
    CHECK("slot restage", slots.stage("1.2.0"));
    CHECK("slot reactivate", slots.request_activation());
    CHECK("slot healthy commit", slots.report_boot_health(true));
    CHECK("slot version committed",
          slots.state().active == Slot::b && slots.state().active_version == "1.2.0" &&
              slots.state().committed);
    CHECK("slot reload", AbUpdateCoordinator(store).initialize("ignored").value->active ==
                             Slot::b);

    AbUpdateCoordinator interrupted_slots(store);
    interrupted_slots.initialize("ignored");
    interrupted_slots.stage("1.3.0");
    interrupted_slots.request_activation();
    AbUpdateCoordinator recovered(store);
    recovered.initialize("ignored");
    CHECK("slot interrupted recovery", recovered.recover_after_interruption());
    CHECK("slot recovered previous",
          recovered.state().active == Slot::b && recovered.state().committed);
    std::filesystem::remove(journal);

#ifdef SMARTVEHICLE_HAS_OPENSSL
    OpenSslCryptoProvider crypto;
    CHECK("sha256", crypto.verify_hash(
                        "abc", "sha256:ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb"
                               "410ff61f20015ad"));
    CHECK("sha256 mismatch", !crypto.verify_hash("abc", "sha256:00"));
    const std::string public_key =
        "-----BEGIN PUBLIC KEY-----\n"
        "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA1dI4m3b1nwAZEuXtrnvK\n"
        "2eQ/xIm4p4vvuJytyaVKPcHyTjcxp/uIwVuJVwzBHn+3MR2QOJop3ipeAZDGwS3t\n"
        "GfNYbb18UvLb4MBGBAtErQYi2T/oVxNN8JCDVAnI4Ans1JZhLopwVYgUaehfTKGX\n"
        "vKQh5T+vdLnHsO1qWQCW5HafS8RfcLrITARcp9DE8YL3jVcv+VBFUMJ4OnEziSS0\n"
        "slYoASyp5uTl17uhMYAkMjEkqvb2weo8eF6wuac6F0+Ll3zAuymTGOVt8MmPtoVW\n"
        "eSXTF802ULEouJgDrpYJYGILhnypfU8SpMOjILMLZ8jTu7ZXGtM5Mw1IawsEChvu\n"
        "HQIDAQAB\n"
        "-----END PUBLIC KEY-----\n";
    const std::string signed_payload = "smartvehicle-signed-payload";
    const std::string valid_signature =
        "6c85c5538d298776d524a8c51d720b909a9dc5dfb9f61a399e593689fdb26ef2"
        "61588b28f35ef028a5d6cb0fd930b5797b6e7d8290be538c9534ca549f74581a"
        "a410c123dc53619850e128773beac2f47395014781a366b6f045ed3e6fa97c197"
        "47f6ec4f0b624179fa4385f31c6ff4d5aae85b69f85ccebdd5b0680ac7b8ccd5"
        "039cd6ae41765ce9b827ca9efdb170e4fde03a25687e3d58d4073571eff462d7b"
        "691adfcfbb2c8982416e031a1cd8d6bc3374ebc295ab2611b12e1765b01dfaa44"
        "f6b9c1a7e30b20d305d2adc3866f8cd319c15c3dbeac3cb2eb505a52ba5efea1"
        "8d09189479a07ea09ce91d5db36182411518feb89aef02958315e00aa2fff";
    CHECK("RSA signature", crypto.verify_signature(signed_payload, valid_signature, public_key));
    CHECK("RSA tamper", !crypto.verify_signature("tampered", valid_signature, public_key));
    CHECK("secure random id", crypto.random_id().size() == 36);
#endif

    std::cout << "PASS " << passed << " checks\n";
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "FAIL after " << passed << " checks: " << error.what() << '\n';
    return 1;
  }
}
