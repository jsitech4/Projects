export type RiskLevel = 'LOW' | 'MODERATE' | 'HIGH' | 'CRITICAL' | 'UNKNOWN';
export type TelemetryInput = { temperatureC: number | null; humidityPct: number | null; relativeMoisturePct: number | null; batteryV: number | null; soilValid: boolean; environmentValid: boolean; sampledAt: Date | null; ingestedAt: Date; batteryCritical?: boolean; faults?: string[] };
export type RiskResult = { level: RiskLevel; ruleVersion: string; factors: string[]; explanation: string };
export type CropRiskThresholds = {
  soilModerate: number;
  soilHigh: number;
  soilCritical: number;
  temperatureHigh?: number;
  humidityLow?: number;
  staleMinutes: number;
  ruleVersion: string;
};

// A transparent baseline for the single monitor. The soil measurement is a calibrated
// relative index, so crop profiles may later replace these with crop-specific limits.
export const DEVICE_BASELINE_RISK_THRESHOLDS: CropRiskThresholds = {
  soilModerate: 50,
  soilHigh: 35,
  soilCritical: 20,
  temperatureHigh: 34,
  humidityLow: 35,
  staleMinutes: 30,
  ruleVersion: 'device-baseline-v1',
};

/**
 * Thresholds are deliberately supplied by the crop/plot profile. The firmware's soil reading is
 * a calibrated relative index, not VWC, so a global crop threshold would be misleading.
 */
export function calculateRisk(t: TelemetryInput, limits: CropRiskThresholds | undefined, now = new Date()): RiskResult {
  const factors: string[] = [];
  if (!limits) return {
    level: 'UNKNOWN', ruleVersion: 'unconfigured-crop-profile',
    factors: ['No field-specific crop risk profile is assigned.'],
    explanation: 'Water-stress risk is not assessed until a crop profile with locally validated relative-moisture thresholds is assigned to this plot.',
  };
  const stale = now.getTime() - (t.sampledAt ?? t.ingestedAt).getTime() > limits.staleMinutes * 60_000;
  if (!t.soilValid || !t.environmentValid || t.relativeMoisturePct === null || stale) {
    if (!t.soilValid) factors.push('Relative soil moisture is invalid or uncalibrated.');
    if (!t.environmentValid) factors.push('Environmental readings are invalid or missing.');
    if (stale) factors.push('The latest reading is stale.');
    return { level: 'UNKNOWN', ruleVersion: limits.ruleVersion, factors, explanation: 'Risk is unknown because sufficient current, valid sensor data is unavailable.' };
  }
  let points = 0;
  if (t.relativeMoisturePct <= limits.soilCritical) { points += 4; factors.push('Relative soil moisture is critically low.'); }
  else if (t.relativeMoisturePct <= limits.soilHigh) { points += 3; factors.push('Relative soil moisture is low.'); }
  else if (t.relativeMoisturePct <= limits.soilModerate) { points += 1; factors.push('Relative soil moisture is below the preferred range.'); }
  if (limits.temperatureHigh !== undefined && (t.temperatureC ?? -Infinity) >= limits.temperatureHigh) { points += 1; factors.push('Temperature is high.'); }
  if (limits.humidityLow !== undefined && (t.humidityPct ?? Infinity) <= limits.humidityLow) { points += 1; factors.push('Humidity is low.'); }
  if (t.batteryCritical) { points += 1; factors.push('Battery is critical; monitoring reliability may be affected.'); }
  if ((t.faults?.length ?? 0) > 0) { points += 1; factors.push('The device reports active faults.'); }
  const level: RiskLevel = points >= 4 ? 'CRITICAL' : points >= 3 ? 'HIGH' : points >= 1 ? 'MODERATE' : 'LOW';
  return { level, ruleVersion: limits.ruleVersion, factors, explanation: factors.length ? factors.join(' ') : 'Current valid readings do not meet the active water-stress thresholds.' };
}
