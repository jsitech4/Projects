#include "Core.h"
#include "Indicators.h"
#include "Pins.h"
#include <cassert>
#include <iostream>
using namespace cws;
static void tick(Indicators &i, JsonDocument &cfg, uint32_t now) {
    host::clock = now;
    i.loop(now, cfg.as<JsonVariantConst>(), "online");
}
static void setup(Indicators &i, JsonDocument &cfg) {
    defaults(cfg);
    cfg["buzzer"]["enabled"] = true;
    host::clock = 1000;
    i.begin();
    tick(i, cfg, 1000);
}
static void expired_operations_do_not_reactivate_after_wrap() {
    Indicators i;
    DynamicJsonDocument cfg(16384);
    setup(i, cfg);
    assert(i.beep(100, cfg.as<JsonVariantConst>()));
    assert(i.ledTest(100));
    tick(i, cfg, 1001);
    assert(host::levels[pins::BUZZER] == HIGH);
    assert(host::duty[0] == 255);
    tick(i, cfg, 1100);
    assert(host::levels[pins::BUZZER] == LOW);
    // The same millisecond value recurs after the 32-bit counter wraps.
    tick(i, cfg, 1001);
    assert(host::levels[pins::BUZZER] == LOW);
    assert(host::duty[0] == 0);
}
static void buzzer_deadline_crosses_wrap() {
    Indicators i;
    DynamicJsonDocument cfg(16384);
    setup(i, cfg);
    host::clock = UINT32_MAX - 49;
    assert(i.beep(100, cfg.as<JsonVariantConst>()));
    tick(i, cfg, 49);
    assert(host::levels[pins::BUZZER] == HIGH);
    tick(i, cfg, 50);
    assert(host::levels[pins::BUZZER] == LOW);
}
static void inactive_polarity_changes_apply_immediately() {
    Indicators i;
    DynamicJsonDocument cfg(16384);
    setup(i, cfg);
    cfg["buzzer"]["active_high"] = false;
    tick(i, cfg, 1001);
    assert(host::levels[pins::BUZZER] == HIGH);
    cfg["buzzer"]["passive"] = true;
    tick(i, cfg, 1002);
    assert(host::attached[pins::BUZZER] == 1);
    assert(host::duty[1] == 255);
    cfg["buzzer"]["enabled"] = false;
    tick(i, cfg, 1003);
    assert(host::attached[pins::BUZZER] == -1);
    assert(host::modes[pins::BUZZER] == INPUT);
}
static void disable_cancels_pending_beep() {
    Indicators i;
    DynamicJsonDocument cfg(16384);
    setup(i, cfg);
    assert(i.beep(100, cfg.as<JsonVariantConst>()));
    tick(i, cfg, 1001);
    cfg["buzzer"]["enabled"] = false;
    tick(i, cfg, 1002);
    cfg["buzzer"]["enabled"] = true;
    tick(i, cfg, 1003);
    assert(host::levels[pins::BUZZER] == LOW);
}
static void mute_and_fault_priority() {
    Indicators i;
    DynamicJsonDocument cfg(16384);
    setup(i, cfg);
    assert(i.beep(100, cfg.as<JsonVariantConst>()));
    i.mute(1000);
    tick(i, cfg, 1001);
    assert(host::levels[pins::BUZZER] == LOW);
    assert(!i.beep(100, cfg.as<JsonVariantConst>()));
    assert(i.ledTest(3000));
    i.loop(1700, cfg.as<JsonVariantConst>(), "fault");
    assert(host::duty[0] == 0);
    cfg["led"]["enabled"] = false;
    i.loop(1800, cfg.as<JsonVariantConst>(), "fault");
    assert(host::duty[0] == 255);
}
int main() {
    expired_operations_do_not_reactivate_after_wrap();
    buzzer_deadline_crosses_wrap();
    inactive_polarity_changes_apply_immediately();
    disable_cancels_pending_beep();
    mute_and_fault_priority();
    std::cout << "5 production indicator adapter tests passed\n";
}
