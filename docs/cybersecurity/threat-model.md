# Cybersecurity and trust boundaries

Trust boundaries exist at application/service, service/middleware, HAL/device, OTA transport, staging storage, and trust-anchor interfaces. Threats include spoofed principals, malformed/unbounded messages, replay/downgrade, malicious artifacts, exhausted queues/storage, compromised adapters, and rollback abuse.

Controls in this milestone: default-deny authorization hook, typed bounded calls, explicit timeout/error handling, digest/signature/trust hooks, target/version policy, actuator deny gate, audit trail, dependency checker, minimal CI permissions, and SBOM. Never store credentials or private keys. Production ports must add authenticated identity, secure key storage, standards-approved crypto, freshness/replay storage, persistent audit, fuzzing, vulnerability scanning, and safety-reviewed actuator gateways.

The work products can support ISO 26262, ISO/SAE 21434, UNECE R155/R156, and ASPICE evidence, but this repository is not certified or compliant by itself.
