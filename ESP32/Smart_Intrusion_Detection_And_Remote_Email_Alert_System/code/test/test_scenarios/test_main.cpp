#include <unity.h>
#include "Control/alarm_policy/alarm_policy.h"
#include "Control/status_indicator_logic/status_indicator_logic.h"

using status_indicator_logic::Mode;

void setUp() {}
void tearDown() {}

void test_battery_gauge_boundaries()
{
  TEST_ASSERT_EQUAL_UINT8(0, status_indicator_logic::batteryBars(false, 100));
  TEST_ASSERT_EQUAL_UINT8(0, status_indicator_logic::batteryBars(true, 0));
  TEST_ASSERT_EQUAL_UINT8(1, status_indicator_logic::batteryBars(true, 1));
  TEST_ASSERT_EQUAL_UINT8(1, status_indicator_logic::batteryBars(true, 25));
  TEST_ASSERT_EQUAL_UINT8(2, status_indicator_logic::batteryBars(true, 26));
  TEST_ASSERT_EQUAL_UINT8(3, status_indicator_logic::batteryBars(true, 51));
  TEST_ASSERT_EQUAL_UINT8(4, status_indicator_logic::batteryBars(true, 76));
  TEST_ASSERT_EQUAL_UINT8(4, status_indicator_logic::batteryBars(true, 100));
}

void test_indicator_priority_and_pulse_patterns()
{
  TEST_ASSERT_EQUAL_INT((int)Mode::Intrusion, (int)status_indicator_logic::selectMode(true, true, true, true, false, false, true, true));
  TEST_ASSERT_EQUAL_INT((int)Mode::CriticalBattery, (int)status_indicator_logic::selectMode(false, true, true, true, false, false, true, true));
  TEST_ASSERT_EQUAL_INT((int)Mode::InternetUnavailable, (int)status_indicator_logic::selectMode(false, false, false, false, true, false, false, false));
  TEST_ASSERT_EQUAL_INT((int)Mode::Healthy, (int)status_indicator_logic::selectMode(false, false, false, false, true, true, false, false));
  TEST_ASSERT_TRUE(status_indicator_logic::isOn(Mode::Healthy, 0));
  TEST_ASSERT_FALSE(status_indicator_logic::isOn(Mode::Healthy, 100));
  TEST_ASSERT_TRUE(status_indicator_logic::isOn(Mode::InternetUnavailable, 240));
  TEST_ASSERT_FALSE(status_indicator_logic::isOn(Mode::InternetUnavailable, 360));
}

void test_disabled_repeat_requires_motion_clear_before_next_event()
{
  TEST_ASSERT_TRUE(alarm_policy::mustClearBeforeAnotherEvent(true, false));
  TEST_ASSERT_FALSE(alarm_policy::shouldStartEvent(true, true, false, true));
  TEST_ASSERT_TRUE(alarm_policy::shouldStartEvent(true, true, false, false));
  TEST_ASSERT_FALSE(alarm_policy::shouldRepeatAtBoundary(true, false));
}

void test_enabled_repeat_continues_the_existing_event()
{
  TEST_ASSERT_FALSE(alarm_policy::mustClearBeforeAnotherEvent(true, true));
  TEST_ASSERT_TRUE(alarm_policy::shouldRepeatAtBoundary(true, true));
  TEST_ASSERT_FALSE(alarm_policy::shouldStartEvent(false, true, false, false));
}

int main(int, char **)
{
  UNITY_BEGIN();
  RUN_TEST(test_battery_gauge_boundaries);
  RUN_TEST(test_indicator_priority_and_pulse_patterns);
  RUN_TEST(test_disabled_repeat_requires_motion_clear_before_next_event);
  RUN_TEST(test_enabled_repeat_continues_the_existing_event);
  return UNITY_END();
}
