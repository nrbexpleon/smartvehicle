# SmartVehicle portable vehicle OS platform

SmartVehicle 0.3 evolves the SDV reference stack into a portable vehicle OS platform. It is a userspace platform over Linux, a POSIX RTOS, Zephyr or FreeRTOS—not a claim of a new safety-certified kernel.

## Runtime plane

- `DeterministicExecutor` provides cooperative periodic dispatch with fixed task-table capacity, stable criticality ordering, execution budgets, bounded dispatch and overrun statistics.
- `HealthSupervisor` provides heartbeat deadlines, bounded restart policy, degraded operation and explicit safe-state requests for exhausted safety-related components.
- `BoundedChannel<T,N>` provides fixed-capacity FIFO transport with backpressure and a high-watermark metric.
- `VehicleMessage` provides a fixed 256-byte payload envelope so message size is bounded and payload storage is embedded.

## Portability plane

`PlatformCapabilities` describes OS family, CPU architecture, cores, memory budget, clock, threading, virtual-memory, crypto, CAN and Ethernet features. Workloads declare requirements and receive explicit compatibility failures before activation. Host capability detection is concrete; RTOS and SoC ports must populate the same contract in board-specific adapters.

## Efficiency contract

The runtime pre-reserves its bounded task table. Channels and message payloads use fixed storage. Dispatch is cooperative, does not spawn a thread per service and executes only a caller-specified number of ready jobs. Integrators must measure WCET, stack use and end-to-end latency on the actual target; host results are not target guarantees.

## Safety boundary

Criticality metadata is scheduling and supervision input only. It does not establish ASIL decomposition, freedom from interference or ISO 26262 compliance. Safe-state requests are decisions that a reviewed target adapter may consume; this platform does not directly actuate a vehicle.

## Production port checklist

1. Implement clock, thread, storage, network, CAN and boot-control adapters.
2. Replace host capability detection with board-generated capabilities.
3. Define CPU, memory, queue, stack and execution budgets per service.
4. Integrate a qualified hypervisor/RTOS where mixed-criticality isolation requires it.
5. Add SOME/IP or DDS transport behind the middleware boundary.
6. Measure deadlines and fault containment on target hardware and HIL.
7. Complete safety, cybersecurity, update and software-process assurance evidence.
