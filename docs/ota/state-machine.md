# OTA update manager

```mermaid
stateDiagram-v2
 [*] --> Idle
 Idle --> Available
 Available --> ValidatingManifest
 ValidatingManifest --> Downloading
 Downloading --> VerifyingIntegrity
 VerifyingIntegrity --> VerifyingSignature
 VerifyingSignature --> CheckingCompatibility
 CheckingCompatibility --> Staging
 Staging --> PreparingInstallation
 PreparingInstallation --> Installing
 Installing --> ActivationRequested
 ActivationRequested --> VerifyingHealth
 VerifyingHealth --> Committed
 VerifyingHealth --> RollingBack: failed health
 RollingBack --> Failed
 Downloading --> Failed: interrupted
 VerifyingIntegrity --> Failed: hash mismatch
 VerifyingSignature --> Failed: trust failure
```

Every transition is audited. Test crypto recognizes only isolated `test-hash:`/`test-signature:` fixtures and is not production cryptography. Authenticated transport, persistent resume, bootloader activation, real A/B slots, secure elements, and power-loss-safe storage are adapter responsibilities not implemented by the host simulator.
