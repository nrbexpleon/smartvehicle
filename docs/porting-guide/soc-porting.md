# Porting to a new SoC

Keep application, service, middleware, and public contracts unchanged. Implement and test OS clock/task/timer/IPC, storage/filesystem, network, crypto/RNG, diagnostics/logging, watchdog, identity/capabilities, device access, power/lifecycle, CAN/Ethernet, NVM, sensors, and board thermal/power adapters. Compose them in a new profile, declare resource limits, then pass contract, timeout, concurrency, power-loss, security, and substitution tests.

The embedded profile is deliberately `runnable:false` and names its CAN adapter `UNIMPLEMENTED`; it is a porting contract, not production support. Vendor SDK includes belong only in a dedicated adapter implementation and must never leak into upper-layer headers.
