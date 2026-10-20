# Safety boundary

The reference application is non-safety monitoring. Physical actuator commands are denied by `DisabledActuatorGate`. A real adapter requires hazard analysis, safety requirements, freedom-from-interference evidence, watchdog/degraded-state design, authenticated command authorization, independent safety mechanisms, and review before enablement. Cloud and OTA failures cannot be runtime dependencies for core local functions.
