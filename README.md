# SmartVehicle SDV Platform

A buildable C++20 service-oriented vehicle platform with strict layer boundaries, versioned service discovery, typed signals, deterministic middleware, authorization, replaceable HALs, Linux SocketCAN, OpenSSL verification, and persistent OTA A/B recovery decisions. Displayed vehicle values remain **simulation data** unless a target integration explicitly states otherwise.

## Quick start

Requirements: CMake 3.20+, C++20 compiler, Make, Python 3, and OpenSSL 3 development headers (`libssl-dev` on Ubuntu).

```bash
./scripts/verify.sh
```

Equivalent commands:

```bash
python3 tools/config_validator/validate.py config/profiles/*.json
python3 tools/dependency_checker/check.py .
cmake --preset host-debug
cmake --build --preset host-debug -j2
ctest --preset host-debug
./build/vehicle_status config/profiles/host-simulation.json
./build/ota_simulation
```

## Useful host stack

- **Vehicle services:** versioned registry/discovery, typed signal reads, events, health, timeouts and default-deny authorization.
- **Real Linux CAN boundary:** `LinuxSocketCan` supports physical SocketCAN or `vcan`, classic CAN frames, bounded payloads and receive timeouts without leaking Linux types upward.
- **Production-library crypto adapter:** OpenSSL 3 EVP provides SHA-256, public-key signature verification and CSPRNG identifiers. Test-only deterministic crypto remains isolated.
- **Persistent OTA recovery model:** an atomic filesystem journal models inactive-slot staging, activation, post-boot health commit, failed-health rollback and interrupted-activation recovery.
- **Safety gate:** physical actuator commands remain disabled by default.

The Vehicle Status executable loads a profile, registers/discovers API 1.0, reads speed, battery state of charge and operating state, publishes events, reports health, replaces simulator A with simulator B, and runs unchanged application logic. The OTA executable demonstrates verified update flow plus persistent A/B success, failed-health rollback and power-loss recovery decisions.

## Architecture

- **L7 application:** consumes only service/platform contracts.
- **L6 services:** signal API, health/error semantics, authorization.
- **L5 middleware:** registry/discovery and bounded in-process pub/sub.
- **L4 platform:** configuration, crypto/time/log/storage contracts.
- **L3 HAL:** signal-provider, SocketCAN and actuator-safety boundaries.
- **L2 base software:** startup validation, composition and resource policy.
- **L1 implementation:** Linux/x86-64 host adapters and simulator.

See [architecture](docs/architecture/overview.md), [integration status](docs/architecture/integration-status.md), [API contract](docs/api-contracts/vehicle-signals.md), [OTA state machine](docs/ota/state-machine.md), [threat model](docs/cybersecurity/threat-model.md), [testing](docs/testing/strategy.md), and [SoC porting](docs/porting-guide/soc-porting.md).

## Add or replace a component

1. Implement the narrow public contract; keep vendor types inside an adapter.
2. Select it through composition/configuration.
3. Add contract, timeout, error, concurrency, authorization and substitution tests.
4. Run `./scripts/verify.sh`; breaking APIs require a new major version.

To add real hardware, bind the HAL contracts in a dedicated adapter and retain the actuator gate until safety review. To use OTA on target, connect `AbUpdateCoordinator` decisions to authenticated boot-control variables and rollback counters; the host journal is not a bootloader.

## Honest scope

SOME/IP, DDS, CAN FD, Automotive Ethernet, RTOS, AUTOSAR Classic/Adaptive, secure-element key custody, a target bootloader and power-loss-qualified installation remain integration targets. The OpenSSL and SocketCAN adapters are concrete host/Linux implementations, but do not establish vehicle production readiness. An architectural mapping is not AUTOSAR compliance or certification. This project does not claim ISO 26262, ISO/SAE 21434, UNECE R155/R156 or ASPICE compliance.

## Security and licensing

CodeQL C++ security analysis, dependency review, Dependabot, SBOM generation and compiler warnings-as-errors run in GitHub Actions. Do not commit secrets or production private keys. See [SECURITY.md](SECURITY.md).

This version is proprietary and source-available. Production, commercial, vehicle, embedded-system, customer or field deployment requires a separate written commercial license. See [LICENSE](LICENSE) or contact `nagaraj.rbhagwat@expleogroup.com` or an authorized representative of Expleo Group. Earlier versions validly released under MIT remain governed by their earlier license.
