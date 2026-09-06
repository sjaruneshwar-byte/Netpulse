# NetPulse

## Distributed Network Monitoring & Fault Detection System

NetPulse is a C++/Linux-based distributed network monitoring system designed to collect system and network telemetry from multiple monitoring agents and detect potential network and node failures through a centralized monitoring server.

The project focuses on networking, Linux system programming, distributed systems, fault detection, and performance monitoring.

---

## Problem Statement

Monitoring a large number of distributed machines manually is difficult and inefficient.

NetPulse aims to provide a centralized system that can:

- Monitor multiple distributed nodes
- Collect system and network telemetry
- Detect node failures using heartbeat mechanisms
- Identify network problems such as high latency, packet loss, and interface failures
- Store historical telemetry
- Display network health through a real-time dashboard

---

## Architecture

```text
                  ┌──────────────────────┐
                  │      Dashboard       │
                  │  Network Health UI   │
                  └──────────┬───────────┘
                             │
                          REST/API
                             │
                             ▼
                  ┌──────────────────────┐
                  │  Monitoring Server   │
                  │                      │
                  │ Telemetry Collector  │
                  │ Fault Detection      │
                  │ Alert Manager        │
                  └──────────┬───────────┘
                             │
                       TCP Communication
                             │
              ┌──────────────┼──────────────┐
              │              │              │
              ▼              ▼              ▼
         ┌─────────┐    ┌─────────┐    ┌─────────┐
         │ Agent 1 │    │ Agent 2 │    │ Agent 3 │
         │  Linux  │    │  Linux  │    │  Linux  │
         └─────────┘    └─────────┘    └─────────┘
