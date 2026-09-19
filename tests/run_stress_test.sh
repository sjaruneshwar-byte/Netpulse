#!/bin/bash

set -e

COUNT=${1:-10}
BASE_ID="STRESS"

PIDS=()

cleanup() {
    echo
    echo "Stopping stress-test agents..."

    for PID in "${PIDS[@]}"; do
        if kill -0 "$PID" 2>/dev/null; then
            kill "$PID" 2>/dev/null || true
        fi
    done

    echo "Stress test stopped."
}

trap cleanup INT TERM EXIT

echo "======================================"
echo " NetPulse Multi-Agent Stress Test"
echo "======================================"
echo "Agents: $COUNT"
echo "Server: 127.0.0.1:9000"
echo

for ((i=1; i<=COUNT; i++)); do
    AGENT_ID=$(printf "%s_%02d" "$BASE_ID" "$i")

    echo "Starting $AGENT_ID..."

    ./agent/agent "$AGENT_ID" > "/tmp/${AGENT_ID}.log" 2>&1 &

    PIDS+=("$!")

    sleep 0.3
done

echo
echo "All $COUNT agents started."
echo
echo "Agent PIDs:"
printf '  %s\n' "${PIDS[@]}"

echo
echo "Press Ctrl+C to stop the stress test."
echo

while true; do
    sleep 5
done
