#include "Configuration.h"
#include "Storage.h"
#include <cassert>
#include <iostream>
using namespace cws;
static String record(const char *id) {
    return String("{\"schema_version\":\"1.0\",\"message_id\":\"") + id +
           "\",\"message_type\":\"telemetry\"}";
}
static void reset() {
    Preferences::reset();
    host::files.clear();
    host::failRename = false;
    host::failRemove = false;
    host::failWrite = false;
    host::mountable = true;
}
static void configuration_power_loss() {
    reset();
    Configuration initial;
    assert(initial.begin());
    DynamicJsonDocument next(16384);
    next.set(initial.data);
    next["name"] = "first";
    assert(initial.commit(next));
    assert(initial.revision() == 1);
    next["name"] = "uncommitted";
    Preferences::failKey = "cws-config/active";
    assert(!initial.commit(next));
    Configuration reboot;
    assert(reboot.begin());
    assert(reboot.data["name"] == "first");
    assert(reboot.revision() == 1);
}
static void configuration_corruption_fallback() {
    reset();
    Configuration initial;
    assert(initial.begin());
    DynamicJsonDocument next(16384);
    next.set(initial.data);
    next["name"] = "known-good";
    assert(initial.commit(next));
    next["name"] = "latest";
    assert(initial.commit(next));
    Preferences::values["cws-config/slot0"].back() ^= 1;
    Configuration reboot;
    assert(reboot.begin());
    assert(reboot.data["name"] == "known-good");
}
static void configuration_rejects_invalid_without_write() {
    reset();
    Configuration config;
    assert(config.begin());
    DynamicJsonDocument next(16384);
    next.set(config.data);
    next["sample_s"] = 0;
    assert(!config.commit(next));
    assert(Preferences::values.empty());
}
static void recovered_slot_failed_commit_stays_uncommitted() {
    configuration_corruption_fallback();
    Configuration recovered;
    assert(recovered.begin());
    DynamicJsonDocument next(16384);
    next.set(recovered.data);
    next["name"] = "must-not-be-promoted";
    Preferences::failKey = "cws-config/active";
    assert(!recovered.commit(next));
    Configuration reboot;
    assert(reboot.begin());
    assert(reboot.data["name"] == "known-good");
    assert(reboot.revision() == 1);
}
static void fallback_pointer_failure_blocks_slot_reuse() {
    reset();
    Configuration initial;
    assert(initial.begin());
    DynamicJsonDocument next(16384);
    next.set(initial.data);
    next["name"] = "known-good";
    assert(initial.commit(next));
    next["name"] = "latest";
    assert(initial.commit(next));
    Preferences::values["cws-config/slot0"].back() ^= 1;
    auto corrupt = Preferences::values["cws-config/slot0"];
    Preferences::failKey = "cws-config/active";
    Configuration recovered;
    assert(!recovered.begin());
    assert(!recovered.healthy);
    assert(recovered.data["name"] == "known-good");
    next.set(recovered.data);
    next["name"] = "candidate";
    Preferences::failKey = "cws-config/active";
    assert(!recovered.commit(next));
    assert(Preferences::values["cws-config/slot0"] == corrupt);
    assert(recovered.commit(next));
    Configuration reboot;
    assert(reboot.begin());
    assert(reboot.data["name"] == "candidate");
}
static void defaults_recovery_failed_commit_stays_uncommitted() {
    reset();
    Configuration initial;
    assert(initial.begin());
    DynamicJsonDocument next(16384);
    next.set(initial.data);
    next["name"] = "corrupted";
    assert(initial.commit(next));
    Preferences::values["cws-config/slot1"].back() ^= 1;
    Configuration recovered;
    assert(!recovered.begin());
    assert(recovered.revision() == 0);
    next.set(recovered.data);
    next["name"] = "must-not-be-promoted";
    // Fail the candidate write after the recovery pointer has selected defaults.
    Preferences::failKey = "cws-config/slot1";
    assert(!recovered.commit(next));
    Configuration reboot;
    assert(reboot.begin());
    assert(reboot.revision() == 0);
    assert(reboot.data["name"] == "Crop monitor");
    assert(reboot.commit(next));
}
static void queue_survives_restart_and_ack() {
    reset();
    Storage s;
    assert(s.begin());
    assert(s.append(record("record-1"), "record-1", 8, false));
    assert(s.pending() == 1);
    Storage reboot;
    assert(reboot.begin());
    String json;
    uint32_t cursor;
    assert(reboot.pendingAfter(0, json, cursor));
    assert(json == record("record-1"));
    assert(reboot.acknowledge("record-1"));
    Storage again;
    assert(again.begin());
    assert(again.pending() == 0);
    assert(!again.acknowledge("unknown"));
    assert(again.purgeAcknowledged());
    assert(host::files.empty());
}
static void queue_interrupted_write() {
    reset();
    Storage s;
    assert(s.begin());
    host::failWrite = true;
    assert(!s.append(record("partial"), "partial", 8, false));
    Storage reboot;
    assert(reboot.begin());
    assert(reboot.pending() == 0);
    assert(!LittleFS.exists("/records/pending.tmp"));
}
static void queue_interrupted_rename() {
    reset();
    Storage s;
    assert(s.begin());
    host::failRename = true;
    assert(!s.append(record("partial"), "partial", 8, false));
    Storage reboot;
    assert(reboot.begin());
    assert(reboot.pending() == 0);
}
static void queue_crc_corruption_preserved() {
    reset();
    Storage s;
    assert(s.begin());
    assert(s.append(record("record-1"), "record-1", 8, false));
    host::files.begin()->second.back() ^= 1;
    Storage reboot;
    assert(reboot.begin());
    assert(!reboot.healthy());
    assert(reboot.pending() == 0);
    assert(!host::files.empty());
}
static void queue_retention_protects_pending() {
    reset();
    Storage s;
    assert(s.begin());
    for (int i = 0; i < 8; i++) {
        String id = String("record-") + i;
        assert(s.append(record(id.c_str()), id, 8, false));
    }
    assert(!s.append(record("overflow"), "overflow", 8, false));
    assert(s.pending() == 8);
    assert(s.acknowledge("record-3"));
    assert(s.append(record("new-record"), "new-record", 8, false));
    assert(s.pending() == 8);
}
static void cursor_not_reused_after_purge() {
    reset();
    Storage s;
    assert(s.begin());
    assert(s.append(record("before"), "before", 8, false));
    String json;
    uint32_t prior;
    assert(s.pendingAfter(0, json, prior));
    assert(s.acknowledge("before"));
    assert(s.purgeAcknowledged());
    Storage reboot;
    assert(reboot.begin());
    assert(reboot.append(record("after"), "after", 8, false));
    uint32_t later;
    assert(reboot.pendingAfter(prior, json, later));
    assert(later > prior);
}
static void ack_delete_failure_does_not_deliver() {
    reset();
    Storage s;
    assert(s.begin());
    assert(s.append(record("record"), "record", 8, false));
    host::failRemove = true;
    assert(!s.acknowledge("record"));
    Storage reboot;
    assert(reboot.begin());
    assert(reboot.pending() == 1);
}
int main() {
    configuration_power_loss();
    configuration_corruption_fallback();
    configuration_rejects_invalid_without_write();
    recovered_slot_failed_commit_stays_uncommitted();
    fallback_pointer_failure_blocks_slot_reuse();
    defaults_recovery_failed_commit_stays_uncommitted();
    queue_survives_restart_and_ack();
    queue_interrupted_write();
    queue_interrupted_rename();
    queue_crc_corruption_preserved();
    queue_retention_protects_pending();
    cursor_not_reused_after_purge();
    ack_delete_failure_does_not_deliver();
    std::cout << "13 production persistence fault-injection tests passed\n";
}
