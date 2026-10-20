# Integration status

| Capability | Status | Evidence and limits |
|---|---|---|
| In-process middleware | Implemented and tested | Deterministic registry, discovery, request path and pub/sub. |
| Linux CAN | Implemented adapter | `LinuxSocketCan` uses Linux PF_CAN/CAN_RAW, bounded classic-CAN payloads, poll timeouts and error mapping. Unit tests run without hardware; a physical interface or `vcan` is required for end-to-end frames. CAN FD is not yet implemented. |
| Production cryptography | Implemented host adapter | OpenSSL 3 EVP performs SHA-256, RSA/ECDSA-compatible public-key verification, and CSPRNG UUID generation. Tests cover a known digest, a valid RSA signature, tampering, and random IDs. Trust-anchor provisioning and secure-element custody remain target responsibilities. |
| OTA A/B decisions | Implemented persistent simulator | Atomic filesystem journal models staging, activation, health commit, failed-boot rollback and interrupted-activation recovery. It is not a bootloader or a power-loss-qualified production filesystem. |
| SOME/IP | Contract target | No implementation or interoperability claim. |
| DDS | Contract target | No implementation or interoperability claim. |
| AUTOSAR Classic/Adaptive | Architectural mapping target | No compliance or certification claim. |
| Secure element/HSM | Adapter target | Keys are passed as public trust anchors; hardware-backed key custody is not implemented. |
| Embedded Linux/ARM64 | Porting profile | Non-runnable until board, watchdog, storage, lifecycle and device adapters pass target tests. |

The useful host path is now: simulated vehicle services + real OpenSSL verification + optional real/vcan SocketCAN + persistent A/B recovery decisions. Safety review and target evidence are still mandatory before physical actuation or production deployment.
