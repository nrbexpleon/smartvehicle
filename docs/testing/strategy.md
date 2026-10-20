# Testing and quality gates

`./scripts/verify.sh` validates both profiles and dependency direction, configures/builds with warnings as errors, runs **47 compiled checks**, executes both simulations, and emits a CycloneDX 1.5 SBOM containing the OpenSSL dependency.

Coverage includes API versions, discovery, duplicate registration, pub/sub, timeout, authorization, malformed/unknown signals, provider substitution, actuator gating, OTA success/rejection/interruption/rollback, SocketCAN input bounds and unopened-channel errors, persistent A/B staging/activation/healthy commit/unhealthy rollback/reload/interrupted recovery, SHA-256, valid RSA signature verification, tamper rejection, and CSPRNG identifiers.

GitHub CodeQL **default setup** is enabled for the repository; a duplicate advanced-configuration workflow is intentionally not stored because GitHub rejects advanced SARIF uploads while default setup is active. GitHub Actions also runs dependency review, Dependabot configuration validation, SBOM generation, packaging, and the compiled quality gates. A physical CAN or `vcan` integration test, bootloader-in-the-loop test, secure-element test, and target power-loss test require appropriate runners and are not simulated as passed.
