#pragma once
#include "Arduino.h"
#include <map>
namespace host {
inline std::map<std::string, std::vector<uint8_t>> files;
inline bool failRename = false;
inline bool failWrite = false;
inline bool mountable = true;
} // namespace host
class File {
    std::string path;
    size_t cursor = 0, entry = 0;
    bool directory = false, valid = false;
    std::vector<std::string> children;

  public:
    File() = default;
    explicit File(const std::string &p, bool dir = false) : path(p), directory(dir), valid(true) {
        if (dir)
            for (auto &f : host::files)
                if (f.first.find(p + "/") == 0)
                    children.push_back(f.first);
    }
    operator bool() const {
        return valid;
    }
    const char *name() const {
        return path.c_str();
    }
    size_t size() const {
        return host::files[path].size();
    }
    size_t read(uint8_t *out, size_t n) {
        auto &v = host::files[path];
        n = std::min(n, v.size() - cursor);
        memcpy(out, v.data() + cursor, n);
        cursor += n;
        return n;
    }
    int read() {
        uint8_t b;
        return read(&b, 1) ? b : -1;
    }
    bool available() const {
        return cursor < host::files[path].size();
    }
    size_t write(const uint8_t *p, size_t n) {
        if (host::failWrite) {
            host::failWrite = false;
            return 0;
        }
        auto &v = host::files[path];
        v.insert(v.end(), p, p + n);
        return n;
    }
    void flush() {}
    void close() {
        valid = false;
    }
    File openNextFile() {
        return entry < children.size() ? File(children[entry++]) : File();
    }
};
class HostFS {
  public:
    bool begin(bool) {
        return host::mountable;
    }
    void end() {}
    bool format() {
        host::files.clear();
        host::mountable = true;
        return true;
    }
    bool mkdir(const char *) {
        return true;
    }
    bool exists(const String &p) {
        return host::files.count(p) > 0;
    }
    bool remove(const String &p) {
        return host::files.erase(p) > 0;
    }
    bool rename(const String &from, const String &to) {
        if (host::failRename) {
            host::failRename = false;
            return false;
        }
        if (!exists(from) || exists(to))
            return false;
        host::files[to] = host::files[from];
        host::files.erase(from);
        return true;
    }
    File open(const String &p, const char *mode = "r") {
        if (p == "/records")
            return File(p, true);
        if (!strcmp(mode, "w"))
            host::files[p].clear();
        if (!exists(p))
            return File();
        return File(p);
    }
    size_t totalBytes() const {
        return 0x1e0000;
    }
    size_t usedBytes() const {
        size_t n = 0;
        for (auto &f : host::files)
            n += f.second.size();
        return n;
    }
};
inline HostFS LittleFS;
