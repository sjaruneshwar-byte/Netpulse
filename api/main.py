from pathlib import Path
import sqlite3

from fastapi import FastAPI, Query
from fastapi.responses import FileResponse
from fastapi.staticfiles import StaticFiles


# --------------------------------------------------
# Paths
# --------------------------------------------------

BASE_DIR = Path(__file__).resolve().parent.parent

DATABASE_PATH = (
    BASE_DIR / "server" / "netpulse.db"
)

DASHBOARD_PATH = (
    BASE_DIR / "dashboard"
)


# --------------------------------------------------
# FastAPI
# --------------------------------------------------

app = FastAPI(
    title="NetPulse API",
    version="1.0.0",
    description="REST API for NetPulse network monitoring"
)


# --------------------------------------------------
# Dashboard static files
# --------------------------------------------------

app.mount(
    "/static",
    StaticFiles(directory=DASHBOARD_PATH),
    name="static"
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
# Dashboard
# --------------------------------------------------

@app.get("/")
def dashboard():
    return FileResponse(
        DASHBOARD_PATH / "index.html"
    )


# --------------------------------------------------
# Health
# --------------------------------------------------

@app.get("/api/health")
def health():
    return {
        "status": "healthy",
        "service": "NetPulse API"
    }


# --------------------------------------------------
# Agents
# --------------------------------------------------

@app.get("/api/agents")
def get_agents():
    connection = get_connection()

    try:
        query = """
            SELECT
                agent_id,
                hostname,
                client_ip,
                status,
                last_seen,
                cpu,
                memory,
                uptime,
                interface_name
            FROM agents
            ORDER BY agent_id
        """

        rows = connection.execute(query).fetchall()

        return [
            dict(row)
            for row in rows
        ]

    finally:
        connection.close()


# --------------------------------------------------
# Telemetry
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
# Faults
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


# --------------------------------------------------
# Dashboard summary
# --------------------------------------------------

@app.get("/api/summary")
def get_summary():
    connection = get_connection()

    try:
        total_agents = connection.execute(
            "SELECT COUNT(*) FROM agents"
        ).fetchone()[0]

        healthy_agents = connection.execute(
            """
            SELECT COUNT(*)
            FROM agents
            WHERE status = 'HEALTHY'
            """
        ).fetchone()[0]

        suspected_agents = connection.execute(
            """
            SELECT COUNT(*)
            FROM agents
            WHERE status = 'SUSPECTED'
            """
        ).fetchone()[0]

        offline_agents = connection.execute(
            """
            SELECT COUNT(*)
            FROM agents
            WHERE status = 'OFFLINE'
            """
        ).fetchone()[0]

        fault_count = connection.execute(
            "SELECT COUNT(*) FROM fault_events"
        ).fetchone()[0]

        telemetry_count = connection.execute(
            "SELECT COUNT(*) FROM telemetry"
        ).fetchone()[0]


        return {
            "total_agents": total_agents,
            "healthy_agents": healthy_agents,
            "suspected_agents": suspected_agents,
            "offline_agents": offline_agents,
            "fault_count": fault_count,
            "telemetry_count": telemetry_count
        }

    finally:
        connection.close()

# --------------------------------------------------
# Topology
# --------------------------------------------------

@app.get("/api/topology")
def get_topology():
    connection = get_connection()

    try:
        rows = connection.execute("""
            SELECT
                agent_id,
                hostname,
                client_ip,
                status
            FROM agents
            ORDER BY agent_id
        """).fetchall()

        agents = [
            {
                "agent_id": row["agent_id"],
                "hostname": row["hostname"],
                "client_ip": row["client_ip"],
                "status": row["status"]
            }
            for row in rows
        ]

        return {
            "server": {
                "name": "NetPulse Server",
                "status": "ONLINE"
            },
            "agents": agents
        }

    finally:
        connection.close()