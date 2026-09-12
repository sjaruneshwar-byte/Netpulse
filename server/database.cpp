#include "database.h"

#include <iostream>
#include <ctime>
#include <sqlite3.h>


Database::Database()
    : db(nullptr)
{
}


Database::~Database()
{
    if (db != nullptr)
    {
        sqlite3_close(
            static_cast<sqlite3*>(db)
        );

        db = nullptr;
    }
}


// --------------------------------------------------
// Initialize database
// --------------------------------------------------

bool Database::initialize(
    const std::string& databasePath)
{
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


    char* errorMessage = nullptr;

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
            << errorMessage
            << "\n";

        sqlite3_free(errorMessage);

        return false;
    }


    // ------------------------------------------------
    // Fault event table
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
            << "Error creating fault table: "
            << errorMessage
            << "\n";

        sqlite3_free(errorMessage);

        return false;
    }


    std::cout
        << "SQLite database initialized successfully.\n";

    return true;
}


// --------------------------------------------------
// Save telemetry
// --------------------------------------------------

bool Database::saveTelemetry(
    const ParsedTelemetry& telemetry)
{
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
        telemetry.timestamp
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
            << "Error inserting telemetry.\n";

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
            << "Error preparing fault insert.\n";

        return false;
    }


    // Fault timestamps currently use the
    // current server time.

    long long timestamp =
        static_cast<long long>(
            std::time(nullptr)
        );


    sqlite3_bind_int64(
        statement,
        1,
        timestamp
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
            << "Error inserting fault event.\n";

        return false;
    }


    return true;
}