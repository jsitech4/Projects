#pragma once
#include "WiFi.h"
#include <cerrno>
constexpr int MSG_DONTWAIT = 1;
inline int send(int fd, const void *data, size_t length, int) {
    auto &s = host::sockets[fd];
    if (s.blocked) {
        errno = EAGAIN;
        return -1;
    }
    if (!s.active) {
        errno = EIO;
        return -1;
    }
    s.output.append((const char *)data, length);
    return int(length);
}
