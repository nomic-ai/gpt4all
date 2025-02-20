#ifndef SYSINFO_H
#define SYSINFO_H

#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>

#if defined(__linux__)
#   include <unistd.h>
#elif defined(__APPLE__)
#   include <sys/types.h>
#   include <sys/sysctl.h>
#elif defined(_WIN32)
#   define WIN32_LEAN_AND_MEAN
#   ifndef NOMINMAX
#       define NOMINMAX
#   endif
#   include <windows.h>
#endif

static long long getSystemTotalRAMInBytes()
{
    long long totalRAM = 0;

#if defined(__linux__)
    std::ifstream file("/proc/meminfo");
    std::string line;
    while (std::getline(file, line)) {
        if (line.find("MemTotal") != std::string::npos) {
            std::string memTotalStr = line.substr(line.find(":") + 1);
            memTotalStr.erase(0, memTotalStr.find_first_not_of(" "));
            memTotalStr = memTotalStr.substr(0, memTotalStr.find(" "));
            totalRAM = std::stoll(memTotalStr) * 1024;  // Convert from KB to bytes
            break;
        }
    }
    file.close();
#elif defined(__APPLE__)
    int mib[2] = {CTL_HW, HW_MEMSIZE};
    size_t length = sizeof(totalRAM);
    sysctl(mib, 2, &totalRAM, &length, NULL, 0);
#elif defined(_WIN32)
    MEMORYSTATUSEX memoryStatus;
    memoryStatus.dwLength = sizeof(memoryStatus);
    GlobalMemoryStatusEx(&memoryStatus);
    totalRAM = memoryStatus.ullTotalPhys;
#endif

    return totalRAM;
}

static double getSystemTotalRAMInGB()
{
    return static_cast<double>(getSystemTotalRAMInBytes()) / (1024 * 1024 * 1024);
}

static std::string getSystemTotalRAMInGBString()
{
    std::stringstream ss;
    ss << std::fixed << std::setprecision(2) << getSystemTotalRAMInGB() << " GB";
    return ss.str();
}

#endif // SYSINFO_H
