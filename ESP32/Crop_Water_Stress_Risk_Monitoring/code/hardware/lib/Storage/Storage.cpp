#include "Storage.h"
#include "Core.h"
#include <algorithm>
namespace cws {
struct RecordHeader {
    uint32_t magic, length, crc, index;
};
static constexpr uint32_t Magic = 0x43575331;
String Storage::path(const Entry &e) const {
    char p[32];
    snprintf(p, sizeof(p), "/records/%08lx.%s", (unsigned long)e.index, e.ack ? "ack" : "rec");
    return p;
}
bool Storage::read(const Entry &e, String &json) {
    File f = LittleFS.open(path(e), "r");
    RecordHeader h{};
    if (!f || f.read((uint8_t *)&h, sizeof(h)) != sizeof(h) || h.magic != Magic ||
        h.index != e.index || h.length > MaxMessage || f.size() != sizeof(h) + h.length)
        return false;
    json = "";
    json.reserve(h.length);
    while (f.available())
        json += (char)f.read();
    return json.length() == h.length && crc32(json.c_str(), json.length()) == h.crc;
}
bool Storage::begin() {
    count = 0;
    next = 1;
    mounted = LittleFS.begin(false);
    if (!mounted) {
        faults++;
        return false;
    }
    if (!counters.begin("cws-queue", false)) {
        mounted = false;
        faults++;
        return false;
    }
    LittleFS.mkdir("/records");
    File dir = LittleFS.open("/records");
    for (File f = dir.openNextFile(); f; f = dir.openNextFile()) {
        String name = f.name();
        int slash = name.lastIndexOf('/');
        if (slash >= 0)
            name = name.substring(slash + 1);
        if (!(name.endsWith(".rec") || name.endsWith(".ack")))
            continue;
        if (name.length() != 12) {
            faults++;
            continue;
        }
        Entry e;
        e.index = strtoul(name.substring(0, 8).c_str(), nullptr, 16);
        e.ack = name.endsWith(".ack");
        next = std::max(next, e.index + 1);
        String json;
        if (!read(e, json)) {
            faults++;
            continue;
        }
        DynamicJsonDocument d(512);
        StaticJsonDocument<128> filter;
        filter["message_id"] = true;
        if (deserializeJson(d, json, DeserializationOption::Filter(filter)) ||
            !identifier(d["message_id"] | "", 79) || count >= MaxRecords) {
            faults++;
            continue;
        }
        strlcpy(e.id, d["message_id"], sizeof(e.id));
        entries[count++] = e;
    }
    std::sort(entries, entries + count,
              [](const Entry &a, const Entry &b) { return a.index < b.index; });
    next = std::max(next, counters.getUInt("next", 1));
    reservedUntil = next;
    for (size_t i = 0; i < count; i++)
        if (entries[i].ack)
            lastAck = entries[i].id;
    if (LittleFS.exists("/records/pending.tmp"))
        LittleFS.remove("/records/pending.tmp");
    return true;
}
bool Storage::formatCommissioning() {
    if (mounted && count)
        return false;
    counters.end();
    LittleFS.end();
    if (!LittleFS.format())
        return false;
    faults = 0;
    return begin();
}
bool Storage::erase(size_t i) {
    if (!LittleFS.remove(path(entries[i]))) {
        faults++;
        return false;
    }
    for (size_t j = i + 1; j < count; j++)
        entries[j - 1] = entries[j];
    count--;
    return true;
}
bool Storage::append(const String &json, const String &id, uint16_t retain, bool dropOldest) {
    if (!mounted || json.length() > MaxMessage || id.length() >= sizeof(entries[0].id) ||
        next == UINT32_MAX) {
        faults++;
        return false;
    }
    for (size_t i = 0; i < count; i++)
        if (id == entries[i].id)
            return true;
    while (count >= std::min<size_t>(retain, MaxRecords)) {
        size_t victim = count;
        for (size_t i = 0; i < count; i++)
            if (entries[i].ack) {
                victim = i;
                break;
            }
        if (victim == count) {
            if (!dropOldest) {
                dropped++;
                return false;
            }
            victim = 0;
            dropped++;
        }
        if (!erase(victim))
            return false;
    }
    if (LittleFS.totalBytes() - LittleFS.usedBytes() < json.length() + 16384) {
        dropped++;
        return false;
    }
    // Allocate cursor ranges before file writes so deletion and reboot cannot reuse history
    // cursors.
    if (next >= reservedUntil) {
        if (next > UINT32_MAX - 64 || counters.putUInt("next", next + 64) != sizeof(uint32_t)) {
            faults++;
            return false;
        }
        reservedUntil = next + 64;
    }
    Entry e;
    e.index = next;
    strlcpy(e.id, id.c_str(), sizeof(e.id));
    RecordHeader h{Magic, uint32_t(json.length()), crc32(json.c_str(), json.length()), e.index};
    File f = LittleFS.open("/records/pending.tmp", "w");
    if (!f) {
        faults++;
        return false;
    }
    bool ok = f.write((uint8_t *)&h, sizeof(h)) == sizeof(h) &&
              f.write((const uint8_t *)json.c_str(), json.length()) == json.length();
    f.flush();
    f.close();
    if (!ok || !LittleFS.rename("/records/pending.tmp", path(e))) {
        faults++;
        return false;
    }
    String verify;
    if (!read(e, verify)) {
        faults++;
        return false;
    }
    entries[count++] = e;
    next++;
    return true;
}
bool Storage::acknowledge(const String &id) {
    for (size_t i = 0; i < count; i++)
        if (id == entries[i].id) {
            if (entries[i].ack)
                return true;
            Entry e = entries[i];
            e.ack = true;
            if (!LittleFS.rename(path(entries[i]), path(e))) {
                faults++;
                return false;
            }
            entries[i] = e;
            lastAck = id;
            return true;
        }
    return false;
}
bool Storage::pendingAfter(uint32_t after, String &json, uint32_t &cursor) {
    for (size_t i = 0; i < count; i++)
        if (!entries[i].ack && entries[i].index > after) {
            cursor = entries[i].index;
            if (read(entries[i], json))
                return true;
            faults++;
        }
    return false;
}
void Storage::history(JsonObject out, uint32_t after, uint8_t limit) {
    auto a = out.createNestedArray("records");
    uint32_t cursor = after;
    bool more = false;
    for (size_t i = 0; i < count; i++) {
        if (entries[i].index <= after)
            continue;
        if (a.size() >= std::min<size_t>(1, limit)) {
            more = true;
            break;
        }
        String json;
        if (!read(entries[i], json)) {
            faults++;
            continue;
        }
        DynamicJsonDocument d(12288);
        if (deserializeJson(d, json)) {
            faults++;
            continue;
        }
        auto row = a.createNestedObject();
        row["index"] = entries[i].index;
        row["acknowledged"] = entries[i].ack;
        row["record"] = d.as<JsonVariant>();
        cursor = entries[i].index;
    }
    out["next_cursor"] = cursor;
    out["more"] = more;
}
uint32_t Storage::pending() const {
    uint32_t n = 0;
    for (size_t i = 0; i < count; i++)
        if (!entries[i].ack)
            n++;
    return n;
}
void Storage::statistics(JsonObject o) const {
    o["mounted"] = mounted;
    o["records"] = count;
    o["pending_records"] = pending();
    o["capacity_records"] = MaxRecords;
    o["capacity_bytes"] = mounted ? LittleFS.totalBytes() : 0;
    o["used_bytes"] = mounted ? LittleFS.usedBytes() : 0;
    o["dropped_records_since_boot"] = dropped;
    o["storage_faults_since_boot"] = faults;
    o["retry_count_since_boot"] = retries;
    o["last_acknowledged_record"] = lastAck;
    for (size_t i = 0; i < count; i++)
        if (!entries[i].ack) {
            o["oldest_pending_record"] = entries[i].id;
            return;
        }
    o["oldest_pending_record"] = nullptr;
}
bool Storage::purgeAcknowledged() {
    for (size_t i = 0; i < count;)
        if (entries[i].ack) {
            if (!erase(i))
                return false;
        } else
            i++;
    return true;
}
} // namespace cws
