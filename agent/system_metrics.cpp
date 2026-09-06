#include "system_metrics.h"

#include <fstream>
#include <sstream>
#include <string>
#include <thread>
#include <chrono>


double getCpuUsage()
{
    std::ifstream file1("/proc/stat");

    if (!file1.is_open())
    {
        return -1.0;
    }

    std::string line;

    std::getline(file1, line);

    std::istringstream stream1(line);

    std::string cpu;

    unsigned long long user1 = 0;
    unsigned long long nice1 = 0;
    unsigned long long system1 = 0;
    unsigned long long idle1 = 0;
    unsigned long long iowait1 = 0;
    unsigned long long irq1 = 0;
    unsigned long long softirq1 = 0;
    unsigned long long steal1 = 0;

    stream1 >> cpu
            >> user1
            >> nice1
            >> system1
            >> idle1
            >> iowait1
            >> irq1
            >> softirq1
            >> steal1;

    unsigned long long idleTime1 =
        idle1 + iowait1;

    unsigned long long totalTime1 =
        user1 +
        nice1 +
        system1 +
        idle1 +
        iowait1 +
        irq1 +
        softirq1 +
        steal1;


    // Wait one second before taking
    // the second CPU measurement.
    std::this_thread::sleep_for(
        std::chrono::seconds(1)
    );


    std::ifstream file2("/proc/stat");

    if (!file2.is_open())
    {
        return -1.0;
    }

    std::getline(file2, line);

    std::istringstream stream2(line);

    unsigned long long user2 = 0;
    unsigned long long nice2 = 0;
    unsigned long long system2 = 0;
    unsigned long long idle2 = 0;
    unsigned long long iowait2 = 0;
    unsigned long long irq2 = 0;
    unsigned long long softirq2 = 0;
    unsigned long long steal2 = 0;

    stream2 >> cpu
            >> user2
            >> nice2
            >> system2
            >> idle2
            >> iowait2
            >> irq2
            >> softirq2
            >> steal2;

    unsigned long long idleTime2 =
        idle2 + iowait2;

    unsigned long long totalTime2 =
        user2 +
        nice2 +
        system2 +
        idle2 +
        iowait2 +
        irq2 +
        softirq2 +
        steal2;


    unsigned long long totalDifference =
        totalTime2 - totalTime1;

    unsigned long long idleDifference =
        idleTime2 - idleTime1;


    if (totalDifference == 0)
    {
        return 0.0;
    }


    double usage =
        100.0 *
        (1.0 -
         static_cast<double>(idleDifference) /
         totalDifference);


    // Protect against tiny numerical
    // anomalies.
    if (usage < 0.0)
    {
        usage = 0.0;
    }

    if (usage > 100.0)
    {
        usage = 100.0;
    }


    return usage;
}

double getMemoryUsage()
{
    std::ifstream file("/proc/meminfo");

    if (!file.is_open())
    {
        return -1.0;
    }

    unsigned long long totalMemory = 0;
    unsigned long long availableMemory = 0;

    std::string key;
    unsigned long long value;
    std::string unit;

    while (file >> key >> value >> unit)
    {
        if (key == "MemTotal:")
        {
            totalMemory = value;
        }
        else if (key == "MemAvailable:")
        {
            availableMemory = value;
        }
    }

    if (totalMemory == 0)
    {
        return -1.0;
    }

    double memoryUsage =
        100.0 *
        (1.0 -
         static_cast<double>(availableMemory) /
         totalMemory);

    return memoryUsage;
}


std::string getUptime()
{
    std::ifstream file("/proc/uptime");

    if (!file.is_open())
    {
        return "Unknown";
    }

    double seconds = 0.0;

    file >> seconds;

    unsigned long long totalSeconds =
        static_cast<unsigned long long>(seconds);

    unsigned long long hours =
        totalSeconds / 3600;

    unsigned long long minutes =
        (totalSeconds % 3600) / 60;

    unsigned long long remainingSeconds =
        totalSeconds % 60;

    std::ostringstream result;

    result << hours
           << "h "
           << minutes
           << "m "
           << remainingSeconds
           << "s";

    return result.str();
}