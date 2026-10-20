# Vehicle Signal API 1.0

ID: `org.smartvehicle.vehicle-signals`; compatibility: same major, provider minor >= consumer minor. `get(principal,id,timeout)` returns `Result<VehicleSignal>` with explicit errors. Values are typed; speed is km/h, battery SoC is %, quality is 0.0–1.0, UTC timestamp records acquisition, sequence is source-local, and validity is `valid|stale|invalid|unavailable`.

The service is thread-safe. Returned values are owned copies. A non-positive timeout fails. Unknown IDs return `not_found`; denied callers return `unauthorized`. Consumers need `signals.read`. The host demo data is simulation-only. Freshness policy is provider-specific and exposed through validity/timestamp. Contract, authorization, timeout, version, and substitution behavior are compiled tests.
