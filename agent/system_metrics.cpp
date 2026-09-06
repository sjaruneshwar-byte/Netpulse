#include "system_metrics.h"

#include <fstream>
#include <sstream>
#include <string>
#include <thread>
#include <chrono>


double getCpuUsage()
{
    std::ifstream file("/proc/stat");

    if (!file.is_open())
    {
        return -1.0;
    }

    std::string line;

    std::getline(file, line);

    std::istringstream stream(line);

    std::string cpu;

    unsigned long long user = 0;
    unsigned long long nice = 0;
    unsigned long long system = 0;
    unsigned long long idle = 0;
    unsigned long long iowait = 0;
    unsigned long long irq = 0;
    unsigned long long softirq = 0;
    unsigned long long steal = 0;

    stream >> cpu
           >> user
           >> nice
           >> system
           >> idle
           >> iowait
           >> irq
           >> softirq
           >> steal;

    unsigned long long idleTime =
        idle + iowait;

    unsigned long long totalTime =
        user +
        nice +
        system +
        idle +
        iowait +
        irq +
        softirq +
        steal;

    // Take first sample.
    std::this_thread::sleep_for(
        std::chrono::seconds(1)
    );

    file.close();

    std::ifstream secondFile("/proc/stat");

    if (!secondFile.is_open())
    {
        return -1.0;
    }

    std::getline(secondFile, line);

    std::istringstream secondStream(line);

    secondStream >> cpu
                 >> user
                 >> nice
                 >> system
                 >> idle
                 >> iowait
                 >> irq
                 >> softirq
                 >> steal;

    unsigned long long secondIdleTime =
        idle + iowait;

    unsigned long long secondTotalTime =
        user +
        nice +
        system +
        idle +
        iowait +
        irq +
        softirq +
        steal;

    unsigned long long totalDifference =
        secondTotalTime - totalTime;

    unsigned long long idleDifference =
        secondIdleTime - idleTime;

    if (totalDifference == 0)
    {
        return 0.0;
    }

    double cpuUsage =
        100.0 *
        (1.0 -
         static_cast<double>(idleDifference) /
         totalDifference);

    return cpuUsage;
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