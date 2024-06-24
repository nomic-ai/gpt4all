#pragma once

class QString;

bool gpt4all_fsync(int fd);
bool gpt4all_fdatasync(int fd);
bool gpt4all_syncdir(const QString &path);
