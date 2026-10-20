# Porting to a new SoC

Keep application, service, middleware, and public contracts unchanged. Implement and test OS clock/task/timer/IPC, storage/filesystem, network, crypto/RNG, diagnostics/logging, watchdog, identity/capabilities, device access, power/lifecycle, CAN/Ethernet, NVM, sensors, and board thermal/power adapters. Compose them in a new profile, declare resource limits, then pass contract, timeout, concurrency, power-loss, security, and substitution tests.

## Reusable Linux pieces

- `LinuxSocketCan` is usable with a physical SocketCAN interface or `vcan`; add CAN FD and OEM signal mapping separately.
- `OpenSslCryptoProvider` supplies SHA-256, public-key signature verification, and CSPRNG IDs. Replace trust-anchor loading with a secure element/HSM adapter when required.
- `FileSlotStore` and `AbUpdateCoordinator` define persistent A/B decisions and recovery semantics. Bind these decisions to the target boot-control API rather than treating the journal as a bootloader.

## Target acceptance checklist

1. Implement watchdog, power/lifecycle, identity, storage and device adapters.
2. Connect A/B decisions to authenticated boot-control variables and monotonic rollback counters.
3. Provision trust anchors through an approved manufacturing/service process.
4. Execute real CAN load, timeout, bus-off, malformed-frame and recovery tests.
5. Execute installation interruption and power-cut campaigns on target storage.
6. Produce threat-analysis, safety-analysis, traceability and independent review evidence.

The embedded profile remains `runnable:false`; it is a porting contract, not production support. Vendor SDK includes belong only in dedicated adapter implementations and must never leak into upper-layer headers.
