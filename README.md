# SmartVehicle Portable Vehicle OS Platform

A buildable C++20 service-oriented vehicle platform with bounded runtime primitives, platform capability negotiation, health supervision, versioned service discovery, typed signals, authorization, replaceable HALs, Linux SocketCAN, OpenSSL verification, and persistent OTA A/B recovery decisions. Displayed vehicle values remain **simulation data** unless a target integration explicitly states otherwise.

SmartVehicle is a userspace vehicle platform over Linux or a suitable RTOS/hypervisor. It is not a new kernel and does not claim safety certification.

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
./build/vehicle_os_demo
```

## Vehicle OS runtime

- **Portable activation:** workloads declare core, memory, clock, threading, virtual-memory, hardware-crypto, CAN and Ethernet requirements. Incompatible targets are rejected explicitly.
- **Bounded executor:** cooperative periodic dispatch has a fixed task-table limit, stable criticality ordering, execution budgets, dispatch caps and overrun statistics.
- **Health supervision:** heartbeat deadlines drive bounded restart, degraded-mode and safe-state decisions according to component criticality.
- **Efficient messaging:** `BoundedChannel<T,N>` provides fixed-capacity FIFO storage and backpressure. `VehicleMessage` embeds a bounded 256-byte payload.
- **Vehicle services:** versioned registry/discovery, typed signal reads, events, health, timeouts and default-deny authorization.
- **Linux CAN boundary:** `LinuxSocketCan` supports physical SocketCAN or `vcan`, classic CAN frames, bounded payloads and receive timeouts without leaking Linux types upward.
- **Production-library crypto:** OpenSSL 3 EVP provides SHA-256, public-key signature verification and CSPRNG identifiers. Test-only deterministic crypto remains isolated.
- **Persistent OTA recovery:** an atomic filesystem journal models inactive-slot staging, activation, post-boot health commit, failed-health rollback and interrupted-activation recovery.
- **Safety gate:** physical actuator commands remain disabled by default.

The Vehicle OS demo performs host capability negotiation, sends a bounded vehicle message, dispatches a periodic ASIL-labelled workload, captures budget metrics and evaluates platform health. Criticality labels are metadata for scheduling and supervision; they do not establish an ASIL claim.

## Architecture

- **L7 application:** consumes only service/platform contracts.
- **L6 services:** signal API, lifecycle, health/error semantics and authorization.
- **L5 middleware:** bounded channels, fixed messages, registry/discovery and pub/sub.
- **L4 runtime/platform:** deterministic executor, health supervisor, capabilities, crypto/time/log/storage contracts.
- **L3 HAL:** signal-provider, SocketCAN and actuator-safety boundaries.
- **L2 base software:** startup validation, composition, resource and recovery policy.
- **L1 implementation:** Linux/x86-64 host adapters and simulator; ARM64/RTOS adapters are explicit ports.

See [vehicle OS architecture](docs/architecture/vehicle-os-platform.md), [architecture overview](docs/architecture/overview.md), [integration status](docs/architecture/integration-status.md), [API contract](docs/api-contracts/vehicle-signals.md), [OTA state machine](docs/ota/state-machine.md), [threat model](docs/cybersecurity/threat-model.md), [testing](docs/testing/strategy.md), and [SoC porting](docs/porting-guide/soc-porting.md).

## Porting

1. Implement narrow clock, thread, storage, transport, CAN, crypto and boot-control contracts; keep vendor types inside adapters.
2. Populate board capabilities and reject workloads that exceed target resources.
3. Select adapters through composition/configuration.
4. Add contract, timeout, error, concurrency, authorization, budget and substitution tests.
5. Measure WCET, stack, memory and end-to-end latency on target hardware.
6. Run `./scripts/verify.sh`; breaking APIs require a new major version.

Retain the actuator gate until safety review. Connect `AbUpdateCoordinator` decisions to authenticated boot-control variables and rollback counters; the host journal is not a bootloader.

## Honest scope

SOME/IP, DDS, CAN FD, Automotive Ethernet/TSN, concrete Zephyr/FreeRTOS/QNX adapters, AUTOSAR Classic/Adaptive, secure-element key custody, a target bootloader and power-loss-qualified installation remain integration targets. Host/Linux implementations do not establish vehicle production readiness. Architectural mapping and criticality metadata are not AUTOSAR or ISO 26262 compliance. This project does not claim ISO 26262, ISO/SAE 21434, UNECE R155/R156 or ASPICE compliance.

## Security and licensing

CodeQL C++ security analysis, dependency review, Dependabot, SBOM generation and compiler warnings-as-errors run in GitHub Actions. Do not commit secrets or production private keys. See [SECURITY.md](SECURITY.md).

This version is proprietary and source-available. Production, commercial, vehicle, embedded-system, customer or field deployment requires a separate written commercial license. See [LICENSE](LICENSE) or contact `nagaraj.rbhagwat@expleogroup.com` or an authorized representative of Expleo Group. Earlier versions validly released under MIT remain governed by their earlier license.
