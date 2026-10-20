#!/usr/bin/env bash
set -euo pipefail
python3 tools/config_validator/validate.py config/profiles/*.json
python3 tools/dependency_checker/check.py .
cmake --preset host-debug
cmake --build --preset host-debug -j2
ctest --preset host-debug
./build/vehicle_status config/profiles/host-simulation.json
./build/ota_simulation
python3 tools/sbom/generate.py
