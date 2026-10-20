# ADR-0001: Host platform foundation

**Status:** Accepted — 2026-10-20

Use C++20 and CMake for the core; standard-library-only host implementations avoid hidden downloads. Cross-layer contracts live under `include/smartvehicle`; the deterministic in-process middleware and simulated HAL prove interfaces without claiming SOME/IP, DDS, AUTOSAR, CAN driver, bootloader, or production-crypto support. Python 3 validates JSON configuration, architecture rules, and CycloneDX SBOM generation. A lightweight compiled test harness is the maintainable equivalent test framework for this dependency-free milestone.

API compatibility is major-equal/minor-provider-at-least. Breaking changes require a new major version. Composition uses explicit constructor injection; globals and singleton hardware access are prohibited.
