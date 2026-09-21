cd ~/projects/NetPulse

cat > README.md <<'EOF'
# NetPulse

NetPulse is a distributed network monitoring platform that collects system and network telemetry from multiple monitoring agents, processes the data through a concurrent C++ server, detects faults and agent failures, persists monitoring history in SQLite, exposes the data through a FastAPI REST API, and visualizes the system through a web dashboard.

## Overview

NetPulse is designed around a distributed monitoring architecture:

```text
+----------------+       +----------------+
|    Agent 01    |       |    Agent 02    |
| CPU / Memory   |       | CPU / Memory   |
| Network stats  |       | Network stats  |
+-------+--------+       +-------+--------+
        |                        |
        |       TCP :9000       |
        +-----------+------------+
                    |
                    v
          +---------------------+
          |   C++ Monitoring    |
          |       Server        |
          +----------+----------+
                     |
             +-------+-------+
             |               |
             v               v
          SQLite       Fault Detection
             |
             v
          FastAPI
             |
             v
       Web Dashboard
