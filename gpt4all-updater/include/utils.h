#pragma once

#include <stdlib.h>
#include <QString>

constexpr uint32_t hash(const char* data) noexcept;
void mountAndExtract(const QString &mountPoint, const QString &saveFilePath, const QString &appBundlePath);
