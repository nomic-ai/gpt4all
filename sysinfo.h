#include <QtCore/QCoreApplication>
#include <QDebug>
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>

#if defined(Q_OS_MAC)
#include <sys/types.h>
#include <sys/sysctl.h>
#endif

#if defined(Q_OS_WIN)
#include <Windows.h>
#endif

QString getSystemTotalRAM()
{
    qint64 totalRAM = 0;

#if defined(Q_OS_LINUX)
    QFile file("/proc/meminfo");
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        QString line = in.readLine();
        while (!line.isNull()) {
            if (line.startsWith("MemTotal")) {
                QStringList parts = line.split(QRegularExpression("\\s+"));
                totalRAM = parts[1].toLongLong() * 1024; // Convert from KB to bytes
                break;
            }
            line = in.readLine();
        }
        file.close();
    }
#elif defined(Q_OS_MAC)
    int mib[2] = {CTL_HW, HW_MEMSIZE};
    size_t length = sizeof(totalRAM);
    sysctl(mib, 2, &totalRAM, &length, NULL, 0);
#elif defined(Q_OS_WIN)
    MEMORYSTATUSEX memoryStatus;
    memoryStatus.dwLength = sizeof(memoryStatus);
    GlobalMemoryStatusEx(&memoryStatus);
    totalRAM = memoryStatus.ullTotalPhys;
#endif

    double totalRAM_GB = static_cast<double>(totalRAM) / (1024 * 1024 * 1024);
    return QString::number(totalRAM_GB, 'f', 2) + " GB";
}
