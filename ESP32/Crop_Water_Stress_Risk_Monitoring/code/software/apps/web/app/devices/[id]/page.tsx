'use client';

import { useEffect, useMemo, useState } from 'react';
import { Line, LineChart, ResponsiveContainer, Tooltip, XAxis, YAxis } from 'recharts';

const api = process.env.NEXT_PUBLIC_API_URL ?? 'http://localhost:4000';
const dateKey = (date: Date) => date.toISOString().slice(0, 10);
const day = (daysAgo = 0) => dateKey(new Date(Date.now() - daysAgo * 86_400_000));
type View = 'overview' | 'fields' | 'devices' | 'history' | 'alerts' | 'reports' | 'settings';
type IconName = 'overview' | 'fields' | 'devices' | 'history' | 'alerts' | 'reports' | 'settings';
type ConfigurationDraft = { name: string; sample_s: number; upload_s: number; health_s: number; ledEnabled: boolean; ledBreathing: boolean; buzzerEnabled: boolean; buzzerAlerts: boolean; retain: number; dropOldest: boolean; durableAck: boolean; };
const views: View[] = ['overview', 'fields', 'devices', 'history', 'alerts', 'reports', 'settings'];

function numberValue(value: unknown, suffix = '', decimals = 1) {
  if (!Number.isFinite(Number(value))) return 'Not available';
  return `${Number(value).toFixed(decimals)}${suffix}`;
}

function shortNumber(value: unknown) { return numberValue(value, '%', 0); }
function shorten(text: string) { return text.replace(/(-?\d+\.\d{1})\d+/g, '$1'); }

function Metric({ label, current, tone = '' }: { label: string; current: string; tone?: string }) {
  return <div className={`metric ${tone}`}><small>{label}</small><strong>{current}</strong></div>;
}

function NavIcon({ name }: { name: IconName }) {
  const shared = { fill: 'none', stroke: 'currentColor', strokeWidth: 1.9, strokeLinecap: 'round' as const, strokeLinejoin: 'round' as const };
  const paths: Record<IconName, React.ReactNode> = {
    overview: <><path {...shared} d="m3 11 9-8 9 8" /><path {...shared} d="M5 10v9h14v-9M9 19v-5h6v5" /></>,
    fields: <><path {...shared} d="M12 21V10" /><path {...shared} d="M12 14C5 14 4 8 4 4c6 0 8 4 8 10ZM12 11c1-5 4-7 8-7 0 5-2 9-8 10" /></>,
    devices: <><rect {...shared} x="6" y="6" width="12" height="12" rx="2" /><path {...shared} d="M9 2v4m3-4v4m3-4v4M9 18v4m3-4v4m3-4v4M2 9h4m-4 3h4m-4 3h4m12-6h4m-4 3h4m-4 3h4" /></>,
    history: <><rect {...shared} x="4" y="4" width="16" height="17" rx="2" /><path {...shared} d="M8 2v4m8-4v4M8 11h8m-8 4h5" /></>,
    alerts: <><path {...shared} d="M18 9a6 6 0 0 0-12 0c0 7-3 7-3 9h18c0-2-3-2-3-9M10 22h4" /></>,
    reports: <><path {...shared} d="M4 20V10m5 10V4m5 16v-7m5 7V7" /></>,
    settings: <><circle {...shared} cx="12" cy="12" r="3" /><path {...shared} d="M19.4 15a1.7 1.7 0 0 0 .34 1.88l.06.06-2.1 2.1-.06-.06a1.7 1.7 0 0 0-1.88-.34 1.7 1.7 0 0 0-1.03 1.55V20h-3v-.1a1.7 1.7 0 0 0-1.03-1.55 1.7 1.7 0 0 0-1.88.34l-.06.06-2.1-2.1.06-.06A1.7 1.7 0 0 0 7.06 14.7 1.7 1.7 0 0 0 5.5 13.67H5.4v-3h.1a1.7 1.7 0 0 0 1.56-1.03 1.7 1.7 0 0 0-.34-1.88l-.06-.06 2.1-2.1.06.06a1.7 1.7 0 0 0 1.88.34 1.7 1.7 0 0 0 1.03-1.55V4.3h3v.1a1.7 1.7 0 0 0 1.03 1.55 1.7 1.7 0 0 0 1.88-.34l.06-.06 2.1 2.1-.06.06a1.7 1.7 0 0 0-.34 1.88 1.7 1.7 0 0 0 1.56 1.03h.1v3h-.1A1.7 1.7 0 0 0 19.4 15Z" /></>,
  };
  return <svg className="nav-icon" viewBox="0 0 24 24" aria-hidden="true">{paths[name]}</svg>;
}

function WeatherIcon({ code }: { code: number }) {
  const common = { fill: 'none', stroke: 'currentColor', strokeWidth: 1.8, strokeLinecap: 'round' as const };
  if (code <= 2) return <svg className="weather-icon sun" viewBox="0 0 24 24" aria-hidden="true"><circle {...common} cx="12" cy="12" r="4" /><path {...common} d="M12 2v2m0 16v2M4.9 4.9l1.4 1.4m11.4 11.4 1.4 1.4M2 12h2m16 0h2M4.9 19.1l1.4-1.4m11.4-11.4 1.4-1.4" /></svg>;
  if (code <= 48) return <svg className="weather-icon cloud" viewBox="0 0 24 24" aria-hidden="true"><path {...common} d="M7 18h10a4 4 0 0 0 .4-8A5.5 5.5 0 0 0 7 8a4.5 4.5 0 0 0 0 10Z" /></svg>;
  return <svg className="weather-icon rain" viewBox="0 0 24 24" aria-hidden="true"><path {...common} d="M7 15h10a4 4 0 0 0 .4-8A5.5 5.5 0 0 0 7 5a4.5 4.5 0 0 0 0 10Z" /><path {...common} d="m8 19-1 3m5-3-1 3m5-3-1 3" /></svg>;
}

function ForecastPanel({ forecast }: { forecast: any }) {
  return <section className="panel forecast"><div className="forecast-heading"><div><p className="eyebrow">Local weather</p><h2>7-day forecast</h2><span>Forecast conditions for the registered farm location</span></div><div className="forecast-badge">Open-Meteo</div></div>{forecast?.configured ? <div className="forecast-days">{forecast.days.map((entry: any) => <div className="forecast-day" key={entry.date}><b>{new Date(`${entry.date}T12:00`).toLocaleDateString([], { weekday: 'short' })}</b><WeatherIcon code={entry.weatherCode} /><strong>{Math.round(entry.maxTemperatureC)} C</strong><small>Rain {entry.rainProbability ?? 0}%</small></div>)}</div> : <div className="weather-setup"><div className="weather-illustration"><WeatherIcon code={1} /></div><div><strong>Farm location not configured</strong><p>Weather forecasts are unavailable until the farm coordinates are registered.</p></div></div>}</section>;
}

function Calendar({ selectedDate, onPick }: { selectedDate: string; onPick: (next: string) => void }) {
  const selected = new Date(`${selectedDate}T12:00:00`);
  const firstDay = new Date(selected.getFullYear(), selected.getMonth(), 1).getDay();
  const totalDays = new Date(selected.getFullYear(), selected.getMonth() + 1, 0).getDate();
  const cells = [...Array(firstDay).fill(0), ...Array.from({ length: totalDays }, (_, index) => index + 1)];
  return <div className="calendar" aria-label="History date picker">
    <div className="calendar-title"><span>{selected.toLocaleDateString([], { month: 'long', year: 'numeric' })}</span><small>Choose a day</small></div>
    <div className="calendar-week">{['Sun', 'Mon', 'Tue', 'Wed', 'Thu', 'Fri', 'Sat'].map((label) => <span key={label}>{label}</span>)}</div>
    <div className="calendar-days">{cells.map((number, index) => {
      if (!number) return <span className="calendar-empty" key={`empty-${index}`} />;
      const next = dateKey(new Date(selected.getFullYear(), selected.getMonth(), number, 12));
      return <button className={next === selectedDate ? 'selected' : ''} onClick={() => onPick(next)} key={next}>{number}</button>;
    })}</div>
  </div>;
}

function Heading({ title, note }: { title: string; note: string }) {
  return <div className="view-heading"><p className="eyebrow">North Field Farm</p><h2>{title}</h2><p>{note}</p></div>;
}

function ThinChart({ rows }: { rows: any[] }) {
  return <ResponsiveContainer width="100%" height={310}><LineChart data={rows}>
    <XAxis dataKey="at" minTickGap={55} />
    <YAxis yAxisId="soil" domain={[0, 100]} unit="%" />
    <YAxis yAxisId="temperature" orientation="right" unit=" C" />
    <Tooltip />
    <Line yAxisId="soil" dataKey="soil" name="Relative soil moisture" stroke="#0a7a42" strokeWidth={1.25} dot={false} />
    <Line yAxisId="temperature" dataKey="temperature" name="Temperature" stroke="#f37916" strokeWidth={1.25} dot={false} />
  </LineChart></ResponsiveContainer>;
}

function HistoryPanel({ history, from, to, setFrom, setTo, page, setPage }: any) {
  const chooseDate = (next: string) => { if (next < from) setFrom(next); setTo(next); setPage(1); };
  return <section className="panel history-panel"><h2>History Logs</h2><Calendar selectedDate={to} onPick={chooseDate} />
    <div className="filters"><label>From<input type="date" value={from} max={to} onChange={(event) => { setFrom(event.target.value); setPage(1); }} /></label><label>To<input type="date" value={to} min={from} max={day()} onChange={(event) => { setTo(event.target.value); setPage(1); }} /></label></div>
    <p className="muted">Field readings and alerts - local time</p>
    <div className="logs">{history?.entries?.length ? history.entries.map((entry: any) => <div className="log" key={entry.id}><i className={entry.type}>o</i><div><strong>{entry.title}</strong><small>{entry.detail} - {new Date(entry.at).toLocaleString()}</small></div></div>) : <p>No records in this range.</p>}</div>
    <footer>Showing {history ? (history.page - 1) * history.limit + 1 : 0}-{history ? Math.min(history.page * history.limit, history.total) : 0} of {history?.total ?? 0}<button disabled={!history?.hasPrevious} onClick={() => setPage(page - 1)}>Previous</button><button disabled={!history?.hasNext} onClick={() => setPage(page + 1)}>Next</button></footer>
  </section>;
}

export default function DevicePage({ params }: { params: Promise<{ id: string }> }) {
  const [id, setId] = useState('');
  const [device, setDevice] = useState<any>();
  const [forecast, setForecast] = useState<any>();
  const [history, setHistory] = useState<any>();
  const [from, setFrom] = useState(day(6));
  const [to, setTo] = useState(day());
  const [page, setPage] = useState(1);
  const [view, setView] = useState<View>('overview');
  const [error, setError] = useState('');
  const [configuration, setConfiguration] = useState<any>();
  const [draft, setDraft] = useState<ConfigurationDraft>();
  const [configurationError, setConfigurationError] = useState('');
  const [configurationCommand, setConfigurationCommand] = useState<any>();
  const [savingConfiguration, setSavingConfiguration] = useState(false);

  useEffect(() => {
    Promise.resolve(params).then((resolved) => { setId(resolved.id); return fetch(`${api}/devices/${resolved.id}`, { credentials: 'include' }); })
      .then(async (response) => response.ok ? setDevice(await response.json()) : setError('Please sign in to view this field.'))
      .catch(() => setError('The dashboard cannot reach the API.'));
  }, [params]);

  useEffect(() => {
    if (!id) return;
    fetch(`${api}/devices/${id}/history?from=${from}&to=${to}&page=${page}&limit=10`, { credentials: 'include' })
      .then((response) => response.ok ? response.json() : null)
      .then((payload) => setHistory(payload && { ...payload, entries: payload.entries.map((entry: any) => ({ ...entry, title: shorten(entry.title), detail: shorten(entry.detail) })) }));
  }, [id, from, to, page]);

  useEffect(() => { fetch(`${api}/weather/forecast`, { credentials: 'include' }).then((response) => response.ok ? response.json() : null).then(setForecast); }, []);

  useEffect(() => {
    if (!id) return;
    fetch(`${api}/devices/${id}/configuration`, { credentials: 'include' }).then(async (response) => response.ok ? response.json() : Promise.reject(new Error('Device configuration is unavailable.'))).then((payload) => { setConfiguration(payload); const c = payload.configuration; if (c) setDraft({ name: c.name ?? id, sample_s: c.sample_s ?? 60, upload_s: c.upload_s ?? 300, health_s: c.health_s ?? 300, ledEnabled: c.led?.enabled ?? true, ledBreathing: c.led?.breathing ?? false, buzzerEnabled: c.buzzer?.enabled ?? false, buzzerAlerts: c.buzzer?.alerts ?? true, retain: c.storage?.retain ?? 256, dropOldest: c.storage?.drop_oldest ?? false, durableAck: c.storage?.durable_ack ?? false }); }).catch((requestError) => setConfigurationError(requestError.message));
  }, [id]);

  useEffect(() => {
    if (!id || !configurationCommand?.requestId || ['APPLIED', 'REJECTED', 'FAILED', 'TIMED_OUT'].includes(configurationCommand.status)) return;
    const timer = window.setInterval(() => fetch(`${api}/devices/${id}/commands/${configurationCommand.requestId}`, { credentials: 'include' }).then((response) => response.ok ? response.json() : null).then((command) => { if (command) setConfigurationCommand(command); }), 2000);
    return () => window.clearInterval(timer);
  }, [id, configurationCommand?.requestId, configurationCommand?.status]);

  useEffect(() => {
    if (!id || configurationCommand?.status !== 'APPLIED') return;
    fetch(`${api}/devices/${id}/configuration`, { credentials: 'include' }).then((response) => response.ok ? response.json() : null).then((payload) => { if (payload?.configuration) { const c = payload.configuration; setConfiguration(payload); setDraft({ name: c.name ?? id, sample_s: c.sample_s ?? 60, upload_s: c.upload_s ?? 300, health_s: c.health_s ?? 300, ledEnabled: c.led?.enabled ?? true, ledBreathing: c.led?.breathing ?? false, buzzerEnabled: c.buzzer?.enabled ?? false, buzzerAlerts: c.buzzer?.alerts ?? true, retain: c.storage?.retain ?? 256, dropOldest: c.storage?.drop_oldest ?? false, durableAck: c.storage?.durable_ack ?? false }); } });
  }, [id, configurationCommand?.status]);

  useEffect(() => {
    const syncViewFromHash = () => {
      const candidate = window.location.hash.slice(1) as View;
      setView(views.includes(candidate) ? candidate : 'overview');
    };
    syncViewFromHash();
    window.addEventListener('hashchange', syncViewFromHash);
    return () => window.removeEventListener('hashchange', syncViewFromHash);
  }, []);

  const rows = useMemo(() => device ? [...device.telemetry].reverse().map((entry: any) => ({ at: new Date(entry.ingestedAt).toLocaleString([], { day: '2-digit', month: 'short', hour: '2-digit', minute: '2-digit' }), soil: entry.relativeMoisturePct, temperature: entry.temperatureC })) : [], [device]);
  const summary = useMemo(() => {
    const measurements = device?.telemetry ?? [];
    const soil = measurements.map((entry: any) => Number(entry.relativeMoisturePct)).filter(Number.isFinite);
    const temperature = measurements.map((entry: any) => Number(entry.temperatureC)).filter(Number.isFinite);
    const average = (values: number[]) => values.length ? values.reduce((total, value) => total + value, 0) / values.length : NaN;
    return { count: measurements.length, averageSoil: average(soil), lowSoil: soil.length ? Math.min(...soil) : NaN, highTemperature: temperature.length ? Math.max(...temperature) : NaN };
  }, [device]);

  if (error) return <p className="error">{error}</p>;
  if (!device) return <p className="loading">Loading your field data...</p>;

  const latest = device.telemetry[0];
  const risk = device.risks[0];
  const farmName = device.plot?.site?.name ?? 'North Field Farm';
  const fieldName = device.plot?.name ?? device.displayName ?? device.id;
  const online = device.lastSeenAt && Date.now() - new Date(device.lastSeenAt).getTime() < 600_000;
  const updateDraft = (key: keyof ConfigurationDraft, value: string | number | boolean) => setDraft((current) => current ? { ...current, [key]: value } : current);
  const saveConfiguration = async (event: React.FormEvent) => {
    event.preventDefault();
    if (!draft || !Number.isInteger(configuration?.revision)) { setConfigurationError('The device has not supplied a current configuration revision.'); return; }
    setSavingConfiguration(true); setConfigurationError('');
    try {
      const response = await fetch(`${api}/devices/${id}/configuration`, { method: 'POST', credentials: 'include', headers: { 'content-type': 'application/json' }, body: JSON.stringify({ expectedRevision: configuration.revision, patch: { name: draft.name, sample_s: draft.sample_s, upload_s: draft.upload_s, health_s: draft.health_s, led: { enabled: draft.ledEnabled, breathing: draft.ledBreathing }, buzzer: { enabled: draft.buzzerEnabled, alerts: draft.buzzerAlerts }, retain: draft.retain, drop_oldest: draft.dropOldest, durable_ack: draft.durableAck } }) });
      const payload = await response.json(); if (!response.ok) throw new Error(payload.message ?? 'Configuration could not be sent.'); setConfigurationCommand(payload);
    } catch (requestError: any) { setConfigurationError(requestError.message ?? 'Configuration could not be sent.'); }
    finally { setSavingConfiguration(false); }
  };
  const navigate = (next: View) => {
    if (window.location.hash !== `#${next}`) window.location.hash = next;
    setView(next);
  };
  const navItem = (key: View, label: string, icon: IconName) => <button className={view === key ? 'on' : ''} onClick={() => navigate(key)}><NavIcon name={icon} /><span>{label}</span></button>;

  const overview = <>
    <section className="metrics">
      <Metric label="Water-stress risk" current={risk?.level ?? 'Unknown'} tone="risk-card" />
      <Metric label="Relative soil moisture" current={shortNumber(latest?.relativeMoisturePct)} tone="soil-card" />
      <Metric label="Temperature" current={numberValue(latest?.temperatureC, ' C')} tone="temperature-card" />
      <Metric label="Battery" current={numberValue(latest?.batteryV, ' V')} tone="battery-card" />
      <Metric label="Solar" current={numberValue(latest?.solarV, ' V')} tone="solar-card" />
      <Metric label="Last update" current={device.lastSeenAt ? new Date(device.lastSeenAt).toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' }) : 'Not available'} tone="update-card" />
    </section>
    <div className="overview-grid"><section className="panel chart"><div className="title"><div><h2>Relative Soil Moisture and Temperature</h2><p>Recorded readings from the field device</p></div><b>Irrigation reference: 30%</b></div><ThinChart rows={rows} /></section><HistoryPanel history={history} from={from} to={to} setFrom={setFrom} setTo={setTo} page={page} setPage={setPage} /></div>
    <div className="overview-bottom"><ForecastPanel forecast={forecast} /><div className="cards"><section className="action"><p>Water-stress assessment</p><h2>Risk level: {risk?.level ?? 'Unknown'}</h2><span>{risk?.explanation ?? 'No validated assessment is available for the current readings.'}</span></section><section className={online ? 'health online' : 'health'}><p>Device health</p><h2>{online ? 'Online' : 'Offline'}</h2><span>{online ? 'Data received normally.' : 'No recent device data received.'}</span></section></div></div>
  </>;

  const fieldView = <div className="single-view"><Heading title="Field overview" note="Current soil, temperature and water-stress condition for this field." /><div className="view-grid three"><section className="panel focus-card"><p>Field</p><h2>{fieldName}</h2><span>{farmName}</span></section><section className="panel focus-card"><p>Water-stress status</p><h2>{risk?.level ?? 'Unknown'}</h2><span>Calculated from the latest validated readings.</span></section><section className="panel focus-card"><p>Latest soil moisture</p><h2>{shortNumber(latest?.relativeMoisturePct)}</h2><span>Irrigation reference: 30%</span></section></div><section className="panel section-chart"><h2>Field moisture trend</h2><p>Recorded moisture and temperature readings for {fieldName}.</p><ThinChart rows={rows} /></section></div>;

  const deviceView = <div className="single-view"><Heading title="Device status" note="Hardware identity, connectivity and electrical condition for the field node." /><div className="view-grid three"><section className="panel focus-card"><p>Connection</p><h2>{online ? 'Online' : 'Offline'}</h2><span>Last seen {device.lastSeenAt ? new Date(device.lastSeenAt).toLocaleString() : 'Never'}</span></section><section className="panel focus-card"><p>Power</p><h2>{numberValue(latest?.batteryV, ' V')}</h2><span>Solar input {numberValue(latest?.solarV, ' V')}</span></section><section className="panel focus-card"><p>Firmware</p><h2>{device.firmwareVersion ?? 'Not recorded'}</h2><span>{device.hardwareModel ?? 'Hardware model not recorded'}</span></section></div><section className="panel device-profile"><h2>Device profile</h2><dl><div><dt>Device ID</dt><dd>{device.id}</dd></div><div><dt>Assigned field</dt><dd>{fieldName}</dd></div><div><dt>Readings loaded</dt><dd>{summary.count}</dd></div></dl></section></div>;

  const historyView = <div className="single-view"><Heading title="Measurement history" note="Filter the record by day, review logged readings, and page through the device history." /><HistoryPanel history={history} from={from} to={to} setFrom={setFrom} setTo={setTo} page={page} setPage={setPage} /></div>;

  const alertsView = <div className="single-view"><Heading title="Alerts and status" note="Current water-stress assessment and device event status." /><div className="view-grid two"><section className="alert-card"><p>Current water-stress result</p><h2>{risk?.level ?? 'Unknown'}</h2><span>{risk?.explanation ?? 'No validated assessment is available for the current readings.'}</span></section><section className={online ? 'health online' : 'health'}><p>Device availability</p><h2>{online ? 'Online and reporting' : 'Offline'}</h2><span>{online ? 'The latest device reading was received within the normal reporting window.' : 'No recent device readings have been received.'}</span></section></div><section className="panel event-panel"><h2>Recent device events</h2>{device.events?.length ? <div className="event-list">{device.events.slice(0, 8).map((event: any) => <div key={event.id}><strong>{event.code ?? 'Device event'}</strong><span>{new Date(event.ingestedAt).toLocaleString()}</span></div>)}</div> : <p className="empty">No device events have been recorded.</p>}</section></div>;

  const reportsView = <div className="single-view"><Heading title="Field reports" note="Summary statistics for the current field-reading record." /><div className="view-grid four"><section className="panel focus-card"><p>Measurements</p><h2>{summary.count}</h2><span>Recorded for this device</span></section><section className="panel focus-card"><p>Average soil moisture</p><h2>{shortNumber(summary.averageSoil)}</h2><span>Across current records</span></section><section className="panel focus-card"><p>Lowest soil moisture</p><h2>{shortNumber(summary.lowSoil)}</h2><span>Across current records</span></section><section className="panel focus-card"><p>Highest temperature</p><h2>{numberValue(summary.highTemperature, ' C')}</h2><span>Across current records</span></section></div><section className="panel report-panel"><h2>Export field readings</h2><p>Download the full field-reading record for analysis and reporting.</p><a className="download" href={`${api}/devices/${id}/telemetry.csv`}>Download field data CSV</a></section></div>;

  const settingsView = <div className="single-view"><Heading title="Device settings" note="Current device configuration and controlled remote updates." /><div className="settings-layout"><section className="panel settings-panel"><h2>{fieldName} device profile</h2><dl><div><dt>Device ID</dt><dd>{device.id}</dd></div><div><dt>Farm</dt><dd>{farmName}</dd></div><div><dt>Model</dt><dd>{device.hardwareModel ?? 'Not recorded'}</dd></div><div><dt>Firmware</dt><dd>{device.firmwareVersion ?? 'Not recorded'}</dd></div><div><dt>Configuration revision</dt><dd>{configuration?.revision ?? 'Not available'}</dd></div><div><dt>Device status</dt><dd>{online ? 'Online' : 'Offline'}</dd></div></dl></section><section className="panel config-panel"><div className="config-heading"><div><p className="eyebrow">Remote configuration</p><h2>Device operation</h2><span>Changes are delivered to the field device and apply only after device confirmation.</span></div>{configurationCommand && <span className={`command-status ${configurationCommand.status?.toLowerCase()}`}>{configurationCommand.status?.toLowerCase()}</span>}</div>{configurationError && <p className="config-error">{configurationError}</p>}{draft ? <form onSubmit={saveConfiguration}><fieldset disabled={savingConfiguration}><div className="config-group"><h3>Identity and reporting</h3><label>Device name<input value={draft.name} maxLength={48} onChange={(event) => updateDraft('name', event.target.value)} required /></label><div className="config-fields"><label>Sample interval (seconds)<input type="number" min="5" max="86400" value={draft.sample_s} onChange={(event) => updateDraft('sample_s', Number(event.target.value))} required /></label><label>Upload interval (seconds)<input type="number" min="10" max="86400" value={draft.upload_s} onChange={(event) => updateDraft('upload_s', Number(event.target.value))} required /></label><label>Health interval (seconds)<input type="number" min="10" max="86400" value={draft.health_s} onChange={(event) => updateDraft('health_s', Number(event.target.value))} required /></label></div></div><div className="config-group"><h3>Indicators and storage</h3><div className="config-checks"><label><input type="checkbox" checked={draft.ledEnabled} onChange={(event) => updateDraft('ledEnabled', event.target.checked)} />LED indicator enabled</label><label><input type="checkbox" checked={draft.ledBreathing} onChange={(event) => updateDraft('ledBreathing', event.target.checked)} />LED breathing effect</label><label><input type="checkbox" checked={draft.buzzerEnabled} onChange={(event) => updateDraft('buzzerEnabled', event.target.checked)} />Buzzer enabled</label><label><input type="checkbox" checked={draft.buzzerAlerts} onChange={(event) => updateDraft('buzzerAlerts', event.target.checked)} />Buzzer alerts enabled</label><label><input type="checkbox" checked={draft.dropOldest} onChange={(event) => updateDraft('dropOldest', event.target.checked)} />Replace oldest stored record</label><label><input type="checkbox" checked={draft.durableAck} onChange={(event) => updateDraft('durableAck', event.target.checked)} />Require durable delivery acknowledgement</label></div><label className="retain-field">Stored-record limit<input type="number" min="8" max="256" value={draft.retain} onChange={(event) => updateDraft('retain', Number(event.target.value))} required /></label></div><div className="config-footer"><p>{configuration?.receivedAt ? `Current configuration received ${new Date(configuration.receivedAt).toLocaleString()}.` : 'Waiting for the device configuration record.'}</p><button type="submit" className="save-config">{savingConfiguration ? 'Sending changes...' : 'Apply device configuration'}</button></div></fieldset></form> : <div className="config-empty"><strong>Configuration has not been received.</strong><p>Connect the device so it can publish its current configuration to the platform.</p></div>}</section></div></div>;

  const content: Record<View, React.ReactNode> = { overview, fields: fieldView, devices: deviceView, history: historyView, alerts: alertsView, reports: reportsView, settings: settingsView };

  return <div className="farm"><aside><button className="brand" onClick={() => navigate('overview')}><span>Farm Water<br />Monitor</span></button><nav>{navItem('overview', 'Overview', 'overview')}{navItem('fields', 'Fields', 'fields')}{navItem('devices', 'Devices', 'devices')}{navItem('history', 'History Logs', 'history')}{navItem('alerts', 'Alerts', 'alerts')}{navItem('reports', 'Reports', 'reports')}{navItem('settings', 'Settings', 'settings')}</nav><small>Field conditions.<br />Recorded clearly.</small></aside><main><header><div className="farm-title"><span className="farm-mark" aria-hidden="true" /><div><h1>{farmName}</h1><p>{view === 'overview' ? 'Field conditions and device status' : fieldName}</p></div></div><div className="date">{from} to {to}</div></header>{content[view]}</main></div>;
}
