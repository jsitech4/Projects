import test from 'node:test';
import assert from 'node:assert/strict';
import { parseDeviceTopic } from '../src/contracts.js';
test('only accepts the six firmware publication topics', () => { assert.deepEqual(parseDeviceTopic('cws/v1/devices/CWS-A1/telemetry'), { deviceId: 'CWS-A1', kind: 'telemetry' }); assert.equal(parseDeviceTopic('cws/v1/devices/CWS-A1/commands'), undefined); });
