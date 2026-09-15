#include "database.h"

#include <iostream>
#include <ctime>
#include <chrono>
#include <mutex>

#include <sqlite3.h>


Database::Database()
    : db(nullptr)
{
}


Database::~Database()
{
    std::lock_guard<std::mutex> lock(
        databaseMutex
    );

    if (db != nullptr)
    {
        sqlite3_close(
            static_cast<sqlite3*>(db)
        );

        db = nullptr;
    }
}


// --------------------------------------------------
// Convert AgentStatus to database string
// --------------------------------------------------

static const char* agentStatusToString(
    AgentStatus status)
{
    switch (status)
    {
        case AgentStatus::HEALTHY:
            return "HEALTHY";

        case AgentStatus::SUSPECTED:
            return "SUSPECTED";

        case AgentStatus::OFFLINE:
            return "OFFLINE";
    }

    return "UNKNOWN";
}


// --------------------------------------------------
// Initialize database
// --------------------------------------------------

bool Database::initialize(
    const std::string& databasePath)
{
    std::lock_guard<std::mutex> lock(
        databaseMutex
    );


    if (db != nullptr)
    {
        return true;
    }


    sqlite3* database = nullptr;


    int result =
        sqlite3_open(
            databasePath.c_str(),
            &database
        );


    if (result != SQLITE_OK)
    {
        std::cerr
            << "Error: Could not open database.\n";


        if (database != nullptr)
        {
            sqlite3_close(database);
        }


        return false;
    }


    db = database;


    // ------------------------------------------------
    // Enable WAL mode
    // ------------------------------------------------

    char* errorMessage = nullptr;


    result =
        sqlite3_exec(
            database,
            "PRAGMA journal_mode=WAL;",
            nullptr,
            nullptr,
            &errorMessage
        );


    if (result != SQLITE_OK)
    {
        std::cerr
            << "Warning: Could not enable WAL mode: "
            << (errorMessage != nullptr
                    ? errorMessage
                    : "unknown error")
            << "\n";


        if (errorMessage != nullptr)
        {
            sqlite3_free(errorMessage);
            errorMessage = nullptr;
        }
    }


    // ------------------------------------------------
    // Set synchronous mode
    // ------------------------------------------------

    result =
        sqlite3_exec(
            database,
            "PRAGMA synchronous=NORMAL;",
            nullptr,
            nullptr,
            &errorMessage
        );


    if (result != SQLITE_OK)
    {
        std::cerr
            << "Warning: Could not set synchronous mode: "
            << (errorMessage != nullptr
                    ? errorMessage
                    : "unknown error")
            << "\n";


        if (errorMessage != nullptr)
        {
            sqlite3_free(errorMessage);
            errorMessage = nullptr;
        }
    }


    // ------------------------------------------------
    // Agents table
    // ------------------------------------------------

    const char* agentsTable = R"(
        CREATE TABLE IF NOT EXISTS agents (
            agent_id TEXT PRIMARY KEY,
            hostname TEXT,
            client_ip TEXT,
            status TEXT NOT NULL,
            last_seen INTEGER NOT NULL,
            cpu REAL,
            memory REAL,
            uptime TEXT,
            interface_name TEXT
        );
    )";


    errorMessage = nullptr;


    result =
        sqlite3_exec(
            database,
            agentsTable,
            nullptr,
            nullptr,
            &errorMessage
        );


    if (result != SQLITE_OK)
    {
        std::cerr
            << "Error creating agents table: "
            << (errorMessage != nullptr
                    ? errorMessage
                    : "unknown error")
            << "\n";


        if (errorMessage != nullptr)
        {
            sqlite3_free(errorMessage);
        }


        return false;
    }


    // ------------------------------------------------
    // Telemetry table
    // ------------------------------------------------

    const char* telemetryTable = R"(
        CREATE TABLE IF NOT EXISTS telemetry (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            timestamp INTEGER NOT NULL,
            agent_id TEXT NOT NULL,
            hostname TEXT,
            interface_name TEXT,
            cpu REAL,
            memory REAL,
            uptime TEXT,
            rx_bps REAL,
            tx_bps REAL,
            rx_pps REAL,
            tx_pps REAL,
            rx_errors INTEGER,
            tx_errors INTEGER,
            rx_drops INTEGER,
            tx_drops INTEGER
        );
    )";


    errorMessage = nullptr;


    result =
        sqlite3_exec(
            database,
            telemetryTable,
            nullptr,
            nullptr,
            &errorMessage
        );


    if (result != SQLITE_OK)
    {
        std::cerr
            << "Error creating telemetry table: "
            << (errorMessage != nullptr
                    ? errorMessage
                    : "unknown error")
            << "\n";


        if (errorMessage != nullptr)
        {
            sqlite3_free(errorMessage);
        }


        return false;
    }


    // ------------------------------------------------
    // Fault events table
    // ------------------------------------------------

    const char* faultTable = R"(
        CREATE TABLE IF NOT EXISTS fault_events (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            timestamp INTEGER NOT NULL,
            agent_id TEXT NOT NULL,
            interface_name TEXT,
            fault_type TEXT NOT NULL,
            severity TEXT NOT NULL,
            description TEXT
        );
    )";


    errorMessage = nullptr;


    result =
        sqlite3_exec(
            database,
            faultTable,
            nullptr,
            nullptr,
            &errorMessage
        );


    if (result != SQLITE_OK)
    {
        std::cerr
            << "Error creating fault_events table: "
            << (errorMessage != nullptr
                    ? errorMessage
                    : "unknown error")
            << "\n";


        if (errorMessage != nullptr)
        {
            sqlite3_free(errorMessage);
        }


        return false;
    }


    // ------------------------------------------------
    // Indexes
    // ------------------------------------------------

    const char* indexes = R"(
        CREATE INDEX IF NOT EXISTS
        idx_agents_status
        ON agents(status);

        CREATE INDEX IF NOT EXISTS
        idx_telemetry_agent_time
        ON telemetry(agent_id, timestamp);

        CREATE INDEX IF NOT EXISTS
        idx_fault_agent_time
        ON fault_events(agent_id, timestamp);
    )";


    errorMessage = nullptr;


    result =
        sqlite3_exec(
            database,
            indexes,
            nullptr,
            nullptr,
            &errorMessage
        );


    if (result != SQLITE_OK)
    {
        std::cerr
            << "Warning: Could not create indexes: "
            << (errorMessage != nullptr
                    ? errorMessage
                    : "unknown error")
            << "\n";


        if (errorMessage != nullptr)
        {
            sqlite3_free(errorMessage);
        }
    }


    std::cout
        << "SQLite database initialized successfully.\n";


    return true;
}


// --------------------------------------------------
// Save / update agent state
// --------------------------------------------------

bool Database::saveAgentState(
    const AgentState& state)
{
    std::lock_guard<std::mutex> lock(
        databaseMutex
    );


    if (db == nullptr)
    {
        return false;
    }


    const char* sql = R"(
        INSERT INTO agents (
            agent_id,
            hostname,
            client_ip,
            status,
            last_seen,
            cpu,
            memory,
            uptime,
            interface_name
        )
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)
        ON CONFLICT(agent_id)
        DO UPDATE SET
            hostname = excluded.hostname,
            client_ip = excluded.client_ip,
            status = excluded.status,
            last_seen = excluded.last_seen,
            cpu = excluded.cpu,
            memory = excluded.memory,
            uptime = excluded.uptime,
            interface_name = excluded.interface_name;
    )";


    sqlite3_stmt* statement = nullptr;


    int result =
        sqlite3_prepare_v2(
            static_cast<sqlite3*>(db),
            sql,
            -1,
            &statement,
            nullptr
        );


    if (result != SQLITE_OK)
    {
        std::cerr
            << "Error preparing agent-state query: "
            << sqlite3_errmsg(
                   static_cast<sqlite3*>(db))
            << "\n";

        return false;
    }


    long long lastSeen =
        std::chrono::duration_cast<
            std::chrono::seconds
        >(
            state.lastSeen.time_since_epoch()
        ).count();


    const char* status =
        agentStatusToString(
            state.status
        );


    sqlite3_bind_text(
        statement,
        1,
        state.agentId.c_str(),
        -1,
        SQLITE_TRANSIENT
    );


    sqlite3_bind_text(
        statement,
        2,
        state.hostname.c_str(),
        -1,
        SQLITE_TRANSIENT
    );


    sqlite3_bind_text(
        statement,
        3,
        state.clientIP.c_str(),
        -1,
        SQLITE_TRANSIENT
    );


    sqlite3_bind_text(
        statement,
        4,
        status,
        -1,
        SQLITE_TRANSIENT
    );


    sqlite3_bind_int64(
        statement,
        5,
        static_cast<sqlite3_int64>(
            lastSeen)
    );


    sqlite3_bind_double(
        statement,
        6,
        state.cpuUsage
    );


    sqlite3_bind_double(
        statement,
        7,
        state.memoryUsage
    );


    sqlite3_bind_text(
        statement,
        8,
        state.uptime.c_str(),
        -1,
        SQLITE_TRANSIENT
    );


    sqlite3_bind_text(
        statement,
        9,
        state.interfaceName.c_str(),
        -1,
        SQLITE_TRANSIENT
    );


    result =
        sqlite3_step(statement);


    sqlite3_finalize(statement);


    if (result != SQLITE_DONE)
    {
        std::cerr
            << "Error saving agent state: "
            << sqlite3_errmsg(
                   static_cast<sqlite3*>(db))
            << "\n";

        return false;
    }


    return true;
}


// --------------------------------------------------
// Update only agent status
// --------------------------------------------------

bool Database::updateAgentStatus(
    const std::string& agentId,
    AgentStatus status)
{
    std::lock_guard<std::mutex> lock(
        databaseMutex
    );


    if (db == nullptr)
    {
        return false;
    }


    const char* sql = R"(
        UPDATE agents
        SET status = ?
        WHERE agent_id = ?;
    )";


    sqlite3_stmt* statement = nullptr;


    int result =
        sqlite3_prepare_v2(
            static_cast<sqlite3*>(db),
            sql,
            -1,
            &statement,
            nullptr
        );


    if (result != SQLITE_OK)
    {
        std::cerr
            << "Error preparing status update: "
            << sqlite3_errmsg(
                   static_cast<sqlite3*>(db))
            << "\n";

        return false;
    }


    const char* statusText =
        agentStatusToString(status);


    sqlite3_bind_text(
        statement,
        1,
        statusText,
        -1,
        SQLITE_TRANSIENT
    );


    sqlite3_bind_text(
        statement,
        2,
        agentId.c_str(),
        -1,
        SQLITE_TRANSIENT
    );


    result =
        sqlite3_step(statement);


    sqlite3_finalize(statement);


    if (result != SQLITE_DONE)
    {
        std::cerr
            << "Error updating agent status: "
            << sqlite3_errmsg(
                   static_cast<sqlite3*>(db))
            << "\n";

        return false;
    }


    return true;
}


// --------------------------------------------------
// Save telemetry
// --------------------------------------------------

bool Database::saveTelemetry(
    const ParsedTelemetry& telemetry)
{
    std::lock_guard<std::mutex> lock(
        databaseMutex
    );


    if (db == nullptr)
    {
        return false;
    }


    const char* sql = R"(
        INSERT INTO telemetry (
            timestamp,
            agent_id,
            hostname,
            interface_name,
            cpu,
            memory,
            uptime,
            rx_bps,
            tx_bps,
            rx_pps,
            tx_pps,
            rx_errors,
            tx_errors,
            rx_drops,
            tx_drops
        )
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);
    )";


    sqlite3_stmt* statement = nullptr;


    int result =
        sqlite3_prepare_v2(
            static_cast<sqlite3*>(db),
            sql,
            -1,
            &statement,
            nullptr
        );


    if (result != SQLITE_OK)
    {
        std::cerr
            << "Error preparing telemetry insert: "
            << sqlite3_errmsg(
                   static_cast<sqlite3*>(db))
            << "\n";

        return false;
    }


    sqlite3_bind_int64(
        statement,
        1,
        static_cast<sqlite3_int64>(
            telemetry.timestamp)
    );


    sqlite3_bind_text(
        statement,
        2,
        telemetry.agentId.c_str(),
        -1,
        SQLITE_TRANSIENT
    );


    sqlite3_bind_text(
        statement,
        3,
        telemetry.hostname.c_str(),
        -1,
        SQLITE_TRANSIENT
    );


    sqlite3_bind_text(
        statement,
        4,
        telemetry.interfaceName.c_str(),
        -1,
        SQLITE_TRANSIENT
    );


    sqlite3_bind_double(
        statement,
        5,
        telemetry.cpuUsage
    );


    sqlite3_bind_double(
        statement,
        6,
        telemetry.memoryUsage
    );


    sqlite3_bind_text(
        statement,
        7,
        telemetry.uptime.c_str(),
        -1,
        SQLITE_TRANSIENT
    );


    sqlite3_bind_double(
        statement,
        8,
        telemetry.rxBytesPerSecond
    );


    sqlite3_bind_double(
        statement,
        9,
        telemetry.txBytesPerSecond
    );


    sqlite3_bind_double(
        statement,
        10,
        telemetry.rxPacketsPerSecond
    );


    sqlite3_bind_double(
        statement,
        11,
        telemetry.txPacketsPerSecond
    );


    sqlite3_bind_int64(
        statement,
        12,
        static_cast<sqlite3_int64>(
            telemetry.rxErrors)
    );


    sqlite3_bind_int64(
        statement,
        13,
        static_cast<sqlite3_int64>(
            telemetry.txErrors)
    );


    sqlite3_bind_int64(
        statement,
        14,
        static_cast<sqlite3_int64>(
            telemetry.rxDrops)
    );


    sqlite3_bind_int64(
        statement,
        15,
        static_cast<sqlite3_int64>(
            telemetry.txDrops)
    );


    result =
        sqlite3_step(statement);


    sqlite3_finalize(statement);


    if (result != SQLITE_DONE)
    {
        std::cerr
            << "Error inserting telemetry: "
            << sqlite3_errmsg(
                   static_cast<sqlite3*>(db))
            << "\n";

        return false;
    }


    return true;
}


// --------------------------------------------------
// Save fault event
// --------------------------------------------------

bool Database::saveFault(
    const FaultEvent& fault)
{
    std::lock_guard<std::mutex> lock(
        databaseMutex
    );


    if (db == nullptr)
    {
        return false;
    }


    const char* sql = R"(
        INSERT INTO fault_events (
            timestamp,
            agent_id,
            interface_name,
            fault_type,
            severity,
            description
        )
        VALUES (?, ?, ?, ?, ?, ?);
    )";


    sqlite3_stmt* statement = nullptr;


    int result =
        sqlite3_prepare_v2(
            static_cast<sqlite3*>(db),
            sql,
            -1,
            &statement,
            nullptr
        );


    if (result != SQLITE_OK)
    {
        std::cerr
            << "Error preparing fault insert: "
            << sqlite3_errmsg(
                   static_cast<sqlite3*>(db))
            << "\n";

        return false;
    }


    long long timestamp =
        static_cast<long long>(
            std::time(nullptr)
        );


    sqlite3_bind_int64(
        statement,
        1,
        static_cast<sqlite3_int64>(
            timestamp)
    );


    sqlite3_bind_text(
        statement,
        2,
        fault.agentId.c_str(),
        -1,
        SQLITE_TRANSIENT
    );


    sqlite3_bind_text(
        statement,
        3,
        fault.interfaceName.c_str(),
        -1,
        SQLITE_TRANSIENT
    );


    sqlite3_bind_text(
        statement,
        4,
        fault.faultType.c_str(),
        -1,
        SQLITE_TRANSIENT
    );


    sqlite3_bind_text(
        statement,
        5,
        severityToString(
            fault.severity),
        -1,
        SQLITE_TRANSIENT
    );


    sqlite3_bind_text(
        statement,
        6,
        fault.description.c_str(),
        -1,
        SQLITE_TRANSIENT
    );


    result =
        sqlite3_step(statement);


    sqlite3_finalize(statement);


    if (result != SQLITE_DONE)
    {
        std::cerr
            << "Error inserting fault event: "
            << sqlite3_errmsg(
                   static_cast<sqlite3*>(db))
            << "\n";

        return false;
    }


    return true;
}