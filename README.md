# SmartVehicle SDV Platform

A buildable C++20 reference platform for service-oriented vehicle software. The milestone proves strict layer boundaries, versioned service discovery, typed signals, deterministic in-process request/events, authorization, HAL substitution, configuration validation, and a secure-lifecycle OTA state machine. All displayed vehicle data is **simulation data**.

## Quick start

Requirements: CMake 3.20+, a C++20 compiler, Make, and Python 3.

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

## Working vertical slice

The Vehicle Status executable validates a profile, registers/discovers API 1.0, reads speed, battery state of charge, and operating state through `IVehicleSignalService`, publishes typed events, reports service health, replaces simulator A with simulator B, and runs unchanged application logic. The OTA executable demonstrates commit, interrupted download, failed post-update health, and simulated rollback decisions.

## Architecture

- **L7 application:** consumes only service/platform contracts.
- **L6 services:** signal API, health/error semantics, authorization.
- **L5 middleware:** service registry/discovery and bounded in-process pub/sub.
- **L4 platform:** configuration, identity/auth, crypto/time/log/storage extension contracts.
- **L3 HAL:** signal-provider and CAN boundaries; physical actuator deny gate.
- **L2 base software:** host startup, validation, composition, resource policy.
- **L1 implementation:** standard-library Linux/x86-64 simulator.

See [architecture](docs/architecture/overview.md), [API contract](docs/api-contracts/vehicle-signals.md), [OTA state machine](docs/ota/state-machine.md), [threat model](docs/cybersecurity/threat-model.md), [safety boundary](docs/safety/boundaries.md), [testing](docs/testing/strategy.md), and [SoC porting](docs/porting-guide/soc-porting.md).

## Add or replace a component

1. Implement the narrow public contract; keep vendor types inside an adapter.
2. Register the implementation and version in composition/configuration.
3. Add contract, timeout, error, concurrency, authorization, and substitution tests.
4. Run `./scripts/verify.sh`; breaking APIs require a new major version.

To replace middleware, implement `IServiceRegistry` and `IEventBus`; service contracts and applications remain unchanged. To add real hardware, implement HAL/platform contracts in a dedicated adapter and retain the default-deny actuator gate until safety review.

## Honest scope

The embedded profile is deliberately non-runnable. SOME/IP, DDS, CAN drivers, Automotive Ethernet, RTOS, AUTOSAR Classic/Adaptive, production cryptography, secure elements, real A/B slots, and bootloader rollback are documented integration targets, not claimed implementations. An adapter or architectural mapping is not AUTOSAR compliance or certification. This project does not claim ISO 26262, ISO/SAE 21434, UNECE R155/R156, or ASPICE compliance.

## Security and licensing

Do not commit secrets or production keys. Test crypto accepts only deterministic `test-*` fixtures. See [SECURITY.md](SECURITY.md). Licensed under MIT.
