import sqlite3
from pathlib import Path

from fastapi import FastAPI, Query


# --------------------------------------------------
# Configuration
# --------------------------------------------------

BASE_DIR = Path(__file__).resolve().parent.parent

DATABASE_PATH = (
    BASE_DIR / "server" / "netpulse.db"
)


# --------------------------------------------------
# FastAPI application
# --------------------------------------------------

app = FastAPI(
    title="NetPulse API",
    version="1.0.0",
    description="REST API for NetPulse network monitoring"
)


# --------------------------------------------------
# Database helper
# --------------------------------------------------

def get_connection():
    connection = sqlite3.connect(
        DATABASE_PATH
    )

    connection.row_factory = sqlite3.Row

    return connection


# --------------------------------------------------
# Health endpoint
# --------------------------------------------------

@app.get("/api/health")
def health():
    return {
        "status": "healthy",
        "service": "NetPulse API"
    }


# --------------------------------------------------
# Agents endpoint
# --------------------------------------------------

@app.get("/api/agents")
def get_agents():
    connection = get_connection()

    try:
        query = """
            SELECT
                agent_id,
                hostname,
                MAX(timestamp) AS last_seen,
                cpu,
                memory
            FROM telemetry
            GROUP BY agent_id
            ORDER BY agent_id
        """

        rows = connection.execute(query).fetchall()

        agents = []

        for row in rows:
            agents.append({
                "agent_id": row["agent_id"],
                "hostname": row["hostname"],
                "last_seen": row["last_seen"],
                "cpu": row["cpu"],
                "memory": row["memory"]
            })

        return agents

    finally:
        connection.close()


# --------------------------------------------------
# Telemetry endpoint
# --------------------------------------------------

@app.get("/api/telemetry")
def get_telemetry(
    agent_id: str | None = Query(
        default=None
    ),
    limit: int = Query(
        default=50,
        ge=1,
        le=1000
    )
):
    connection = get_connection()

    try:
        if agent_id is None:
            query = """
                SELECT *
                FROM telemetry
                ORDER BY id DESC
                LIMIT ?
            """

            rows = connection.execute(
                query,
                (limit,)
            ).fetchall()

        else:
            query = """
                SELECT *
                FROM telemetry
                WHERE agent_id = ?
                ORDER BY id DESC
                LIMIT ?
            """

            rows = connection.execute(
                query,
                (agent_id, limit)
            ).fetchall()


        return [
            dict(row)
            for row in rows
        ]

    finally:
        connection.close()


# --------------------------------------------------
# Fault endpoint
# --------------------------------------------------

@app.get("/api/faults")
def get_faults(
    agent_id: str | None = Query(
        default=None
    ),
    limit: int = Query(
        default=50,
        ge=1,
        le=1000
    )
):
    connection = get_connection()

    try:
        if agent_id is None:
            query = """
                SELECT *
                FROM fault_events
                ORDER BY id DESC
                LIMIT ?
            """

            rows = connection.execute(
                query,
                (limit,)
            ).fetchall()

        else:
            query = """
                SELECT *
                FROM fault_events
                WHERE agent_id = ?
                ORDER BY id DESC
                LIMIT ?
            """

            rows = connection.execute(
                query,
                (agent_id, limit)
            ).fetchall()


        return [
            dict(row)
            for row in rows
        ]

    finally:
        connection.close()