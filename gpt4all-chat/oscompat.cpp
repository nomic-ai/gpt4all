#include "oscompat.h"

#include <QByteArray>
#include <QString>
#include <QtGlobal>

#ifdef Q_OS_WIN32
#   define WIN32_LEAN_AND_MEAN
#   ifndef NOMINMAX
#      define NOMINMAX
#   endif
#   include <windows.h>
#   include <errno.h>
#else
#   include <fcntl.h> // for open, fcntl
#   ifndef Q_OS_DARWIN
#       include <unistd.h> // for fsync, fdatasync
#   endif
#endif

bool gpt4all_fsync(int fd) {
#if defined(Q_OS_WIN32)
    HANDLE handle = HANDLE(_get_osfhandle(fd));
    if (handle == INVALID_HANDLE_VALUE) {
        errno = EBADF;
        return false;
    }

    if (FlushFileBuffers(handle))
        return true;

    DWORD error = GetLastError();
    switch (error) {
    case ERROR_ACCESS_DENIED: // read-only file
        return true;
    case ERROR_INVALID_HANDLE: // not a regular file
        errno = EINVAL;
    default:
        errno = EIO;
    }

    return false;
#elif defined(Q_OS_DARWIN)
    return fcntl(fd, F_FULLFSYNC, 0) == 0;
#else
    return fsync(fd) == 0;
#endif
}

bool gpt4all_fdatasync(int fd) {
#if defined(Q_OS_WIN32) || defined(Q_OS_DARWIN)
    return gpt4all_fsync(fd);
#else
    return fdatasync(fd) == 0;
#endif
}

bool gpt4all_syncdir(const QString &path) {
#if defined(Q_OS_WIN32)
    (void)path; // cannot sync a directory on Windows
    return true;
#else
    int fd = open(path.toLocal8Bit().constData(), O_RDONLY | O_DIRECTORY);
    if (fd == -1) return false;
    bool ok = gpt4all_fdatasync(fd);
    close(fd);
    return ok;
#endif
}
