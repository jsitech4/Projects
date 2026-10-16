import test from 'node:test';
import assert from 'node:assert/strict';
import { calculateRisk, DEVICE_BASELINE_RISK_THRESHOLDS } from '../src/risk.js';
const base = { temperatureC: 25, humidityPct: 55, relativeMoisturePct: 60, batteryV: 4, soilValid: true, environmentValid: true, sampledAt: new Date(), ingestedAt: new Date() };
const profile = { soilModerate: 45, soilHigh: 30, soilCritical: 15, temperatureHigh: 34, humidityLow: 30, staleMinutes: 30, ruleVersion: 'maize-local-v1' };
test('requires a field-specific crop profile and returns unknown for invalid or stale telemetry', () => { assert.equal(calculateRisk(base, undefined).level, 'UNKNOWN'); assert.equal(calculateRisk({ ...base, soilValid: false }, profile).level, 'UNKNOWN'); assert.equal(calculateRisk({ ...base, sampledAt: new Date(Date.now() - 31 * 60_000) }, profile).level, 'UNKNOWN'); });
test('explains critical low relative moisture', () => { const result = calculateRisk({ ...base, relativeMoisturePct: 12 }, profile); assert.equal(result.level, 'CRITICAL'); assert.match(result.explanation, /critically low/); assert.equal(result.ruleVersion, 'maize-local-v1'); });
test('the single-device baseline assesses calibrated sensor telemetry', () => { const result = calculateRisk(base, DEVICE_BASELINE_RISK_THRESHOLDS); assert.equal(result.level, 'LOW'); assert.equal(result.ruleVersion, 'device-baseline-v1'); });
