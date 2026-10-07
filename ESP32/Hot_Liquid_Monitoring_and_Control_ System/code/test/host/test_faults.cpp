#include <Arduino.h>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

#include "Pins.h"
#include "device_manager/device_manager.h"
#include "led_indicator/led_indicator.h"
#include "buzzer/buzzer.h"
#include "load_relay/load_relay.h"
#include "temp_sensor/temp_sensor.h"
#include "ultrasonic_sensor/ultrasonic_sensor.h"

namespace simulated
{
  unsigned long now = 0;
  float temperature = 50.0f;
  float distance = 22.5f;
  bool temperatureValid = true;
  bool distanceValid = true;
  int temperatureReads = 0;
  int distanceReads = 0;
  uint8_t pins[256]{};
  uint32_t pwm[16]{};
  unsigned assertions = 0;
}

unsigned long millis() { return simulated::now; }
void pinMode(uint8_t, uint8_t) {}
void digitalWrite(uint8_t pin, uint8_t value) { simulated::pins[pin] = value; }
int digitalRead(uint8_t pin) { return simulated::pins[pin]; }
double ledcSetup(uint8_t, double frequency, uint8_t) { return frequency; }
void ledcAttachPin(uint8_t, uint8_t) {}
void ledcWrite(uint8_t channel, uint32_t duty) { simulated::pwm[channel] = duty; }

namespace temp_sensor
{
  float getTemperatureC() { ++simulated::temperatureReads; return simulated::temperature; }
  bool isValid() { return simulated::temperatureValid; }
}

namespace ultrasonic_sensor
{
  float getDistanceCm() { ++simulated::distanceReads; return simulated::distance; }
  bool isValid() { return simulated::distanceValid; }
}

#define CHECK(condition) do { \
  ++simulated::assertions; \
  if (!(condition)) { \
    std::cerr << __func__ << ':' << __LINE__ << ": " << #condition << '\n'; \
    std::exit(EXIT_FAILURE); \
  } \
} while (false)

using namespace device_manager;

static void updateIndicators()
{
  led_indicator::update();
  buzzer::update();
}

static void sample()
{
  simulated::now += 1000;
  device_manager::update();
  updateIndicators();
}

static void reset()
{
  simulated::now = 0;
  simulated::temperature = 50.0f;
  simulated::distance = 22.5f;
  simulated::temperatureValid = true;
  simulated::distanceValid = true;
  load_relay::begin();
  device_manager::begin();
  led_indicator::begin();
  buzzer::begin();
  sample();
  CHECK(!hasFault());
  CHECK(!getSnapshot().faultClearable);
}

static void checkFault(bool tank, bool temperature)
{
  const auto snapshot = getSnapshot();
  CHECK(snapshot.tankLatchTriggered == tank);
  CHECK(snapshot.temperatureLatchTriggered == temperature);
  CHECK(snapshot.faultLatched == (tank || temperature));
  CHECK(hasFault() == (tank || temperature));
  CHECK(snapshot.automaticRelayDemand == (tank || temperature));
  CHECK(isAutomaticRelayDemand() == (tank || temperature));
  CHECK(snapshot.relayOn == (tank || temperature || snapshot.manualRelayRequest));
  CHECK(load_relay::isOn() == snapshot.relayOn);
  CHECK(bool(simulated::pins[Pins::RELAY]) == snapshot.relayOn);
}

static void allFourThresholdsLatchAtEquality()
{
  for (int fault = 0; fault < 4; ++fault)
  {
    reset();
    const bool tank = fault < 2;
    if (tank)
    {
      setRelayLatchMode(fault == 0 ? LATCH_AT_FULL : LATCH_AT_LOW);
      simulated::distance = fault == 0 ? 8.5f : 33.0f; // Exactly 90% and 20%.
    }
    else
    {
      setTemperatureLatchMode(fault == 2 ? TEMP_LATCH_AT_HIGH : TEMP_LATCH_AT_LOW);
      simulated::temperature = fault == 2 ? 80.0f : 30.0f;
    }
    sample();
    checkFault(tank, !tank);
    CHECK(!getSnapshot().faultClearable);
    CHECK(!clearFault());
    clearAutomaticLatches();
    checkFault(tank, !tank);

    simulated::temperature = 50.0f;
    simulated::distance = 22.5f;
    sample();
    checkFault(tank, !tank);
    CHECK(getSnapshot().faultClearable);
    sample();
    checkFault(tank, !tank);
    CHECK(clearFault());
    checkFault(false, false);
    CHECK(!getSnapshot().faultClearable);
  }
}

static void onlySelectedModesCreateFaults()
{
  reset();
  simulated::temperature = 100.0f;
  simulated::distance = 5.0f;
  sample();
  checkFault(false, false);
  setRelayLatchMode(LATCH_AT_LOW);
  setTemperatureLatchMode(TEMP_LATCH_AT_LOW);
  sample();
  checkFault(false, false);

  reset();
  simulated::temperature = 10.0f;
  simulated::distance = 40.0f;
  sample();
  setRelayLatchMode(LATCH_AT_FULL);
  setTemperatureLatchMode(TEMP_LATCH_AT_HIGH);
  sample();
  checkFault(false, false);
}

static void settingsAndDisabledModesPreserveFaults()
{
  reset();
  setRelayLatchMode(LATCH_AT_FULL);
  setTemperatureLatchMode(TEMP_LATCH_AT_HIGH);
  simulated::temperature = 85.0f;
  simulated::distance = 5.0f;
  sample();
  checkFault(true, true);
  CHECK(setTemperatureThresholds(10.0f, 100.0f));
  setLevelThresholds(95.0f, 10.0f);
  CHECK(setTankDistances(1.0f, 50.0f));
  checkFault(true, true);
  setRelayLatchMode(LATCH_OFF);
  setTemperatureLatchMode(TEMP_LATCH_OFF);
  checkFault(true, true);
  CHECK(applySettings(0.0f, 100.0f, 0.0f, 110.0f, LATCH_OFF, TEMP_LATCH_OFF));
  sample();
  checkFault(true, true);
  CHECK(clearFault());
  checkFault(false, false);
}

static void clearingBothFaultsIsAtomic()
{
  reset();
  setRelayLatchMode(LATCH_AT_FULL);
  setTemperatureLatchMode(TEMP_LATCH_AT_HIGH);
  simulated::temperature = 85.0f;
  simulated::distance = 5.0f;
  sample();
  simulated::temperature = 50.0f;
  sample();
  CHECK(!clearFault());
  checkFault(true, true);
  simulated::distance = 22.5f;
  simulated::temperature = 85.0f;
  sample();
  CHECK(!clearFault());
  checkFault(true, true);
  simulated::temperature = 50.0f;
  CHECK(clearFault());
  checkFault(false, false);
}

static void manualRelayRequestSurvivesClear()
{
  reset();
  setManualRelayRequest(true);
  updateIndicators();
  CHECK(!hasFault());
  CHECK(led_indicator::isOn());
  CHECK(!buzzer::isAlarmRunning());
  setTemperatureLatchMode(TEMP_LATCH_AT_HIGH);
  simulated::temperature = 80.0f;
  sample();
  setManualRelayRequest(false);
  checkFault(false, true);
  setManualRelayRequest(true);
  simulated::temperature = 50.0f;
  CHECK(clearFault());
  checkFault(false, false);
  CHECK(getSnapshot().manualRelayRequest);
  CHECK(getSnapshot().relayRequested);
  CHECK(load_relay::isOn());
  setManualRelayRequest(false);
  CHECK(!load_relay::isOn());
}

static void invalidLatchedSensorsBlockClear()
{
  for (bool tank : {false, true})
  {
    reset();
    if (tank)
    {
      setRelayLatchMode(LATCH_AT_FULL);
      simulated::distance = 5.0f;
    }
    else
    {
      setTemperatureLatchMode(TEMP_LATCH_AT_HIGH);
      simulated::temperature = 85.0f;
    }
    sample();
    simulated::distance = 22.5f;
    simulated::temperature = 50.0f;
    (tank ? simulated::distanceValid : simulated::temperatureValid) = false;
    sample();
    CHECK(!getSnapshot().faultClearable);
    CHECK(!clearFault());
    checkFault(tank, !tank);
    // Turning a mode off must not bypass the invalid-source guard.
    setRelayLatchMode(LATCH_OFF);
    setTemperatureLatchMode(TEMP_LATCH_OFF);
    CHECK(!clearFault());
    (tank ? simulated::distanceValid : simulated::temperatureValid) = true;
    for (float invalid : {std::numeric_limits<float>::quiet_NaN(),
                          std::numeric_limits<float>::infinity()})
    {
      (tank ? simulated::distance : simulated::temperature) = invalid;
      CHECK(!clearFault());
      checkFault(tank, !tank);
    }
    simulated::distance = 22.5f;
    simulated::temperature = 50.0f;
    CHECK(clearFault());
    checkFault(false, false);
  }
}

static void clearingRefreshesInputsBeforeDeciding()
{
  reset();
  setTemperatureLatchMode(TEMP_LATCH_AT_HIGH);
  setRelayLatchMode(LATCH_AT_FULL);
  simulated::temperature = 85.0f;
  sample();
  simulated::temperature = 50.0f;
  sample();
  CHECK(getSnapshot().faultClearable);
  // No clock advance or manager update: the clear command must resample.
  simulated::distance = 5.0f;
  const int temperatureReads = simulated::temperatureReads;
  const int distanceReads = simulated::distanceReads;
  CHECK(!clearFault());
  CHECK(simulated::temperatureReads > temperatureReads);
  CHECK(simulated::distanceReads > distanceReads);
  checkFault(true, true);
  CHECK(!getSnapshot().faultClearable);
  simulated::distance = 22.5f;
  CHECK(clearFault());
  checkFault(false, false);
}

static void indicatorsTrackTheLatchedFault()
{
  reset();
  for (int i = 0; i < 5; ++i)
  {
    simulated::now += 250;
    updateIndicators();
    CHECK(led_indicator::isOn());
    CHECK(simulated::pwm[0] == 255);
    CHECK(!buzzer::isActive());
  }
  setTemperatureLatchMode(TEMP_LATCH_AT_HIGH);
  simulated::temperature = 85.0f;
  sample();
  const unsigned long faultStart = simulated::now;
  CHECK(led_indicator::isOn());
  CHECK(buzzer::isAlarmRunning());
  CHECK(buzzer::isActive());
  CHECK(simulated::pins[Pins::BUZZER] == HIGH);
  simulated::now = faultStart + 179;
  updateIndicators();
  CHECK(buzzer::isActive());
  simulated::now = faultStart + 180;
  updateIndicators();
  CHECK(!buzzer::isActive());
  simulated::now = faultStart + 360;
  updateIndicators();
  CHECK(buzzer::isActive());
  simulated::now = faultStart + 499;
  updateIndicators();
  CHECK(led_indicator::isOn());
  simulated::now = faultStart + 500;
  updateIndicators();
  CHECK(!led_indicator::isOn());
  CHECK(simulated::pwm[0] == 0);
  simulated::now = faultStart + 999;
  updateIndicators();
  CHECK(!led_indicator::isOn());
  simulated::now = faultStart + 1000;
  updateIndicators();
  CHECK(led_indicator::isOn());

  simulated::temperature = 50.0f;
  sample();
  CHECK(hasFault());
  CHECK(buzzer::isAlarmRunning());
  CHECK(clearFault());
  updateIndicators();
  CHECK(led_indicator::isOn());
  CHECK(simulated::pwm[0] == 255);
  CHECK(!buzzer::isAlarmRunning());
  CHECK(!buzzer::isActive());
  CHECK(simulated::pins[Pins::BUZZER] == LOW);
  sample();
  CHECK(led_indicator::isOn());
  CHECK(!buzzer::isActive());
}

static void standaloneStartupBeepStillExpires()
{
  reset();
  buzzer::beep(120);
  CHECK(buzzer::isActive());
  simulated::now += 119;
  buzzer::update();
  CHECK(buzzer::isActive());
  ++simulated::now;
  buzzer::update();
  CHECK(!buzzer::isActive());
}

int main()
{
  allFourThresholdsLatchAtEquality();
  onlySelectedModesCreateFaults();
  settingsAndDisabledModesPreserveFaults();
  clearingBothFaultsIsAtomic();
  manualRelayRequestSurvivesClear();
  invalidLatchedSensorsBlockClear();
  clearingRefreshesInputsBeforeDeciding();
  indicatorsTrackTheLatchedFault();
  standaloneStartupBeepStillExpires();
  std::cout << "PASS: 9 fault/indicator scenarios (" << simulated::assertions << " assertions)\n";
}
