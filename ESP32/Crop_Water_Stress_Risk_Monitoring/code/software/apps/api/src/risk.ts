export type RiskLevel = 'LOW' | 'MODERATE' | 'HIGH' | 'CRITICAL' | 'UNKNOWN';
export type TelemetryInput = { temperatureC: number | null; humidityPct: number | null; relativeMoisturePct: number | null; batteryV: number | null; soilValid: boolean; environmentValid: boolean; sampledAt: Date | null; ingestedAt: Date; batteryCritical?: boolean; faults?: string[] };
export type RiskResult = { level: RiskLevel; ruleVersion: string; factors: string[]; explanation: string };
export const DEFAULT_THRESHOLDS = { soilModerate: 45, soilHigh: 30, soilCritical: 15, temperatureHigh: 34, humidityLow: 30, staleMinutes: 30 };
export function calculateRisk(t: TelemetryInput, limits = DEFAULT_THRESHOLDS, now = new Date()): RiskResult {
  const factors: string[] = [];
  const stale = now.getTime() - (t.sampledAt ?? t.ingestedAt).getTime() > limits.staleMinutes * 60_000;
  if (!t.soilValid || !t.environmentValid || t.relativeMoisturePct === null || stale) {
    if (!t.soilValid) factors.push('Relative soil moisture is invalid or uncalibrated.');
    if (!t.environmentValid) factors.push('Environmental readings are invalid or missing.');
    if (stale) factors.push('The latest reading is stale.');
    return { level: 'UNKNOWN', ruleVersion: 'risk-v1', factors, explanation: 'Risk is unknown because sufficient current, valid sensor data is unavailable.' };
  }
  let points = 0;
  if (t.relativeMoisturePct <= limits.soilCritical) { points += 4; factors.push('Relative soil moisture is critically low.'); }
  else if (t.relativeMoisturePct <= limits.soilHigh) { points += 3; factors.push('Relative soil moisture is low.'); }
  else if (t.relativeMoisturePct <= limits.soilModerate) { points += 1; factors.push('Relative soil moisture is below the preferred range.'); }
  if ((t.temperatureC ?? -Infinity) >= limits.temperatureHigh) { points += 1; factors.push('Temperature is high.'); }
  if ((t.humidityPct ?? Infinity) <= limits.humidityLow) { points += 1; factors.push('Humidity is low.'); }
  if (t.batteryCritical) { points += 1; factors.push('Battery is critical; monitoring reliability may be affected.'); }
  if ((t.faults?.length ?? 0) > 0) { points += 1; factors.push('The device reports active faults.'); }
  const level: RiskLevel = points >= 4 ? 'CRITICAL' : points >= 3 ? 'HIGH' : points >= 1 ? 'MODERATE' : 'LOW';
  return { level, ruleVersion: 'risk-v1', factors, explanation: factors.length ? factors.join(' ') : 'Current valid readings do not meet configured water-stress risk thresholds.' };
}
