import test from 'node:test';
import assert from 'node:assert/strict';
import { calculateRisk } from '../src/risk.js';
const base = { temperatureC: 25, humidityPct: 55, relativeMoisturePct: 60, batteryV: 4, soilValid: true, environmentValid: true, sampledAt: new Date(), ingestedAt: new Date() };
test('returns unknown for invalid or stale telemetry', () => { assert.equal(calculateRisk({ ...base, soilValid: false }).level, 'UNKNOWN'); assert.equal(calculateRisk({ ...base, sampledAt: new Date(Date.now() - 31 * 60_000) }).level, 'UNKNOWN'); });
test('explains critical low relative moisture', () => { const result = calculateRisk({ ...base, relativeMoisturePct: 12 }); assert.equal(result.level, 'CRITICAL'); assert.match(result.explanation, /critically low/); });
