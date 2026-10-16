"use client";

import { useCallback, useEffect, useMemo, useState } from "react";
import {
  Area,
  AreaChart,
  CartesianGrid,
  ResponsiveContainer,
  Tooltip,
  XAxis,
  YAxis,
} from "recharts";
import "./device.css";

const api = process.env.NEXT_PUBLIC_API_URL ?? "http://localhost:4000";
type Draft = { upload_s: number; ledEnabled: boolean; buzzerEnabled: boolean };
type IconName = "leaf" | "drop" | "thermometer" | "battery" | "sun" | "antenna";

function reading(input: unknown, unit = "", decimals = 1) {
  return Number.isFinite(Number(input))
    ? `${Number(input).toFixed(decimals)}${unit}`
    : "—";
}
function ago(input?: string | null) {
  if (!input) return "Unavailable";
  const minutes = Math.max(
    0,
    Math.round((Date.now() - new Date(input).getTime()) / 60_000),
  );
  return minutes < 1
    ? "Just now"
    : minutes === 1
      ? "1 minute ago"
      : `${minutes} minutes ago`;
}
function dateTime(input?: string | null) {
  return input
    ? new Intl.DateTimeFormat(undefined, {
        month: "short",
        day: "numeric",
        hour: "2-digit",
        minute: "2-digit",
      }).format(new Date(input))
    : "Unavailable";
}
function dateInputValue(date: Date) {
  const local = new Date(date.getTime() - date.getTimezoneOffset() * 60_000);
  return local.toISOString().slice(0, 10);
}
function initialHistoryRange() {
  const to = new Date();
  const from = new Date(to);
  from.setDate(from.getDate() - 6);
  return { from: dateInputValue(from), to: dateInputValue(to) };
}
function rounded(text: string) {
  return text.replace(/(-?\d+\.\d)\d+/g, "$1");
}
function uptime(input: unknown) {
  const ms = Number(input);
  if (!Number.isFinite(ms) || ms < 0) return null;
  const total = Math.floor(ms / 1000);
  const days = Math.floor(total / 86400);
  const hours = Math.floor((total % 86400) / 3600);
  const minutes = Math.floor((total % 3600) / 60);
  return days
    ? `${days}d ${hours}h`
    : hours
      ? `${hours}h ${minutes}m`
      : `${minutes}m`;
}

function Icon({ name }: { name: IconName }) {
  const art: Record<IconName, React.ReactNode> = {
    leaf: (
      <>
        <path d="M20.2 3.7C12 3.4 5.5 6.2 4.3 12.4c-.8 4.2 2.3 7.5 6.5 7.5 6.1 0 8.9-7.5 9.4-16.2Z" />
        <path d="M4.1 20.3c3.4-5.8 7.6-8.6 12.5-10.5" />
      </>
    ),
    drop: (
      <>
        <path d="M12 2.9S5.5 10.2 5.5 14.7a6.5 6.5 0 0 0 13 0C18.5 10.2 12 2.9 12 2.9Z" />
        <path d="M9.1 15.2c.4 1.4 1.3 2.3 2.7 2.6" />
      </>
    ),
    thermometer: (
      <>
        <path d="M10 14.7V5a2 2 0 0 1 4 0v9.7a4.5 4.5 0 1 1-4 0Z" />
        <path d="M12 9v7" />
      </>
    ),
    battery: (
      <>
        <rect x="5.5" y="4" width="13" height="16" rx="2" />
        <path d="M9.5 2.4h5" />
        <path d="M9 16h6" />
      </>
    ),
    sun: (
      <>
        <circle cx="12" cy="12" r="4" />
        <path d="M12 2v2M12 20v2M2 12h2M20 12h2M4.9 4.9l1.4 1.4M17.7 17.7l1.4 1.4M19.1 4.9l-1.4 1.4M6.3 17.7l-1.4 1.4" />
      </>
    ),
    antenna: (
      <>
        <path d="M12 21V11" />
        <path d="M8.7 17.6a4.7 4.7 0 0 1 0-6.7M15.3 17.6a4.7 4.7 0 0 0 0-6.7M5.5 20.5a8.7 8.7 0 0 1 0-12.3M18.5 20.5a8.7 8.7 0 0 0 0-12.3" />
      </>
    ),
  };
  return (
    <svg viewBox="0 0 24 24" aria-hidden="true">
      <g
        fill="none"
        stroke="currentColor"
        strokeWidth="1.85"
        strokeLinecap="round"
        strokeLinejoin="round"
      >
        {art[name]}
      </g>
    </svg>
  );
}
function Enclosure() {
  return (
    <svg
      className="enclosure"
      viewBox="0 0 150 238"
      aria-label="Crop water monitor enclosure"
      role="img"
    >
      <rect
        x="17"
        y="12"
        width="116"
        height="158"
        rx="13"
        fill="#edf1ef"
        stroke="#b8c0bd"
        strokeWidth="2"
      />
      <circle cx="31" cy="29" r="4" fill="#4c5554" />
      <circle cx="119" cy="29" r="4" fill="#4c5554" />
      <circle cx="31" cy="151" r="4" fill="#4c5554" />
      <circle cx="119" cy="151" r="4" fill="#4c5554" />
      <path d="M85 73c-21 7-32 23-30 43 22-4 33-20 30-43Z" fill="#315a46" />
      <path d="M55 116c10-15 18-24 30-32" stroke="#ffffff" strokeWidth="1.5" />
      <path
        d="M38 169v24m13-24v43m14-43v24m28-24v43m14-43v24"
        stroke="#293430"
        strokeWidth="9"
      />
      <path
        d="M34 190h8m9 18h8m2-18h8m28 18h8m2-18h8"
        stroke="#59615e"
        strokeWidth="4"
      />
      <rect x="29" y="166" width="18" height="14" rx="3" fill="#3c4543" />
      <rect x="51" y="166" width="18" height="14" rx="3" fill="#3c4543" />
      <rect x="89" y="166" width="18" height="14" rx="3" fill="#3c4543" />
      <rect x="111" y="166" width="18" height="14" rx="3" fill="#3c4543" />
    </svg>
  );
}
function Metric({
  icon,
  label,
  value,
  tone = "green",
}: {
  icon: IconName;
  label: string;
  value: string;
  tone?: string;
}) {
  return (
    <article className={`metric ${tone}`}>
      <Icon name={icon} />
      <div>
        <span>{label}</span>
        <strong>{value}</strong>
      </div>
    </article>
  );
}
function status(valid: boolean | undefined, fallback: boolean) {
  return valid === false ? "Fault" : fallback ? "OK" : "Unavailable";
}
function riskLabel(level: string) {
  return level.charAt(0) + level.slice(1).toLowerCase();
}

export default function DevicePage({
  params,
}: {
  params: Promise<{ id: string }>;
}) {
  const [id, setId] = useState("");
  const [device, setDevice] = useState<any>();
  const [history, setHistory] = useState<any>();
  const [configuration, setConfiguration] = useState<any>();
  const [draft, setDraft] = useState<Draft>();
  const [error, setError] = useState("");
  const [saving, setSaving] = useState(false);
  const [command, setCommand] = useState<any>();
  const [fallbackRefreshSeconds, setFallbackRefreshSeconds] = useState(1);
  const [historyOpen, setHistoryOpen] = useState(false);
  const [historyRange, setHistoryRange] = useState(initialHistoryRange);
  const [historyQuery, setHistoryQuery] = useState(initialHistoryRange);
  const [historyPage, setHistoryPage] = useState(1);
  const [savedHistory, setSavedHistory] = useState<any>();
  const [historyLoading, setHistoryLoading] = useState(false);
  const [historyError, setHistoryError] = useState("");
  const load = useCallback(async (deviceId: string) => {
    try {
      const request = {
        credentials: "include" as const,
        cache: "no-store" as const,
      };
      const [deviceResponse, historyResponse, configurationResponse] =
        await Promise.all([
          fetch(`${api}/devices/${deviceId}`, request),
          fetch(`${api}/devices/${deviceId}/history?limit=12`, request),
          fetch(`${api}/devices/${deviceId}/configuration`, request),
        ]);
      if (!deviceResponse.ok)
        throw new Error(
          deviceResponse.status === 401
            ? "Please sign in to view this device."
            : "This device could not be found.",
        );
      const [nextDevice, nextHistory, nextConfiguration] = await Promise.all([
        deviceResponse.json(),
        historyResponse.ok ? historyResponse.json() : null,
        configurationResponse.ok ? configurationResponse.json() : null,
      ]);
      setDevice(nextDevice);
      setHistory(nextHistory);
      setConfiguration(nextConfiguration);
      const c = nextConfiguration?.configuration;
      if (Number.isInteger(c?.upload_s))
        setDraft({
          upload_s: c.upload_s,
          ledEnabled: c.led?.enabled !== false,
          buzzerEnabled: c.buzzer?.enabled === true,
        });
      else setDraft(undefined);
      setError("");
    } catch (requestError: any) {
      setError(requestError.message ?? "The dashboard cannot reach the API.");
    }
  }, []);
  const refreshTelemetry = useCallback(async (deviceId: string) => {
    try {
      const request = {
        credentials: "include" as const,
        cache: "no-store" as const,
      };
      const [deviceResponse, historyResponse] = await Promise.all([
        fetch(`${api}/devices/${deviceId}`, request),
        fetch(`${api}/devices/${deviceId}/history?limit=12`, request),
      ]);
      if (!deviceResponse.ok)
        throw new Error(
          deviceResponse.status === 401
            ? "Please sign in to view this device."
            : "This device could not be found.",
        );
      setDevice(await deviceResponse.json());
      setHistory(historyResponse.ok ? await historyResponse.json() : null);
      setError("");
    } catch (requestError: any) {
      setError(requestError.message ?? "The dashboard cannot reach the API.");
    }
  }, []);
  useEffect(() => {
    Promise.resolve(params).then(({ id: deviceId }) => {
      setId(deviceId);
      load(deviceId);
    });
  }, [params, load]);
  useEffect(() => {
    const stored = Number(
      window.localStorage.getItem("cws-dashboard-refresh-seconds"),
    );
    if ([1, 5, 15, 30, 60].includes(stored)) setFallbackRefreshSeconds(stored);
  }, []);
  useEffect(() => {
    window.localStorage.setItem(
      "cws-dashboard-refresh-seconds",
      String(fallbackRefreshSeconds),
    );
  }, [fallbackRefreshSeconds]);
  useEffect(() => {
    if (!id) return;
    let refreshing = false;
    const refresh = async () => {
      if (document.visibilityState !== "visible" || refreshing) return;
      refreshing = true;
      try {
        await refreshTelemetry(id);
      } finally {
        refreshing = false;
      }
    };
    const refreshOnFocus = () => void refresh();
    const timer = window.setInterval(
      refreshOnFocus,
      fallbackRefreshSeconds * 1_000,
    );
    window.addEventListener("focus", refreshOnFocus);
    document.addEventListener("visibilitychange", refreshOnFocus);
    return () => {
      window.clearInterval(timer);
      window.removeEventListener("focus", refreshOnFocus);
      document.removeEventListener("visibilitychange", refreshOnFocus);
    };
  }, [id, refreshTelemetry, fallbackRefreshSeconds]);
  useEffect(() => {
    if (!id) return;
    const stream = new EventSource(`${api}/devices/${id}/live`, {
      withCredentials: true,
    });
    const onTelemetry = () => void refreshTelemetry(id);
    stream.addEventListener("telemetry", onTelemetry);
    return () => {
      stream.removeEventListener("telemetry", onTelemetry);
      stream.close();
    };
  }, [id, refreshTelemetry]);
  const loadSavedHistory = useCallback(async () => {
    if (!id) return;
    setHistoryLoading(true);
    setHistoryError("");
    try {
      const query = new URLSearchParams({
        from: historyQuery.from,
        to: historyQuery.to,
        page: String(historyPage),
        limit: "25",
      });
      const response = await fetch(
        `${api}/devices/${id}/telemetry-history?${query}`,
        { credentials: "include", cache: "no-store" },
      );
      const payload = await response.json();
      if (!response.ok)
        throw new Error(payload.message ?? "Could not load saved readings.");
      setSavedHistory(payload);
    } catch (requestError: any) {
      setHistoryError(requestError.message ?? "Could not load saved readings.");
    } finally {
      setHistoryLoading(false);
    }
  }, [id, historyPage, historyQuery]);
  useEffect(() => {
    if (historyOpen) void loadSavedHistory();
  }, [historyOpen, loadSavedHistory]);
  const downloadHistory = async () => {
    if (!id) return;
    setHistoryError("");
    try {
      const query = new URLSearchParams(historyQuery);
      const response = await fetch(
        `${api}/devices/${id}/telemetry.csv?${query}`,
        { credentials: "include" },
      );
      if (!response.ok) throw new Error("Could not prepare the CSV file.");
      const url = URL.createObjectURL(await response.blob());
      const link = document.createElement("a");
      link.href = url;
      link.download = `${id}-telemetry-${historyQuery.from}-to-${historyQuery.to}.csv`;
      link.click();
      URL.revokeObjectURL(url);
    } catch (requestError: any) {
      setHistoryError(
        requestError.message ?? "Could not download the CSV file.",
      );
    }
  };
  useEffect(() => {
    if (
      !id ||
      !command?.requestId ||
      ["APPLIED", "REJECTED", "FAILED", "TIMED_OUT"].includes(command.status)
    )
      return;
    const timer = window.setInterval(async () => {
      const response = await fetch(
        `${api}/devices/${id}/commands/${command.requestId}`,
        { credentials: "include" },
      );
      if (response.ok) setCommand(await response.json());
    }, 2_500);
    return () => window.clearInterval(timer);
  }, [id, command?.requestId, command?.status]);
  const chart = useMemo(
    () =>
      (device?.telemetry ?? [])
        .slice()
        .reverse()
        .map((entry: any, index: number) => ({
          // Both sensor values come from the same telemetry message. A shared
          // timestamp keeps them on the same vertical time guide in the chart.
          timestamp: Number.isFinite(Date.parse(entry.ingestedAt))
            ? Date.parse(entry.ingestedAt)
            : index,
          moisture: entry.relativeMoisturePct,
          temperature: entry.temperatureC,
        })),
    [device],
  );
  const save = async (event: React.FormEvent) => {
    event.preventDefault();
    if (!draft || !Number.isInteger(configuration?.revision)) return;
    setSaving(true);
    try {
      const response = await fetch(`${api}/devices/${id}/configuration`, {
        method: "POST",
        credentials: "include",
        headers: { "content-type": "application/json" },
        body: JSON.stringify({
          expectedRevision: configuration.revision,
          patch: {
            sample_s: draft.upload_s,
            upload_s: draft.upload_s,
            led: { enabled: draft.ledEnabled },
            buzzer: { enabled: draft.buzzerEnabled },
            storage: { drop_oldest: true, durable_ack: true },
          },
        }),
      });
      const payload = await response.json();
      if (!response.ok)
        throw new Error(payload.message ?? "Could not send the configuration.");
      setCommand(payload);
    } catch (requestError: any) {
      setCommand({
        status: "FAILED",
        message: requestError.message ?? "Could not send the configuration.",
      });
    } finally {
      setSaving(false);
    }
  };
  if (error)
    return (
      <main className="monitor-error">
        <h1>Crop Water Monitor</h1>
        <p>{error}</p>
        <a href={error.startsWith("Please sign in") ? "/login" : "/devices"}>
          {error.startsWith("Please sign in") ? "Sign in" : "Return to monitor"}
        </a>
      </main>
    );
  if (!device)
    return (
      <main className="monitor-loading">
        <span />
        <p>Loading Crop Water Monitor…</p>
      </main>
    );
  const latest = device.telemetry?.[0];
  const payload = latest?.raw ?? {};
  const raw = payload.measurements ?? payload;
  const valid = latest?.valid ?? raw?.valid ?? {};
  const online = Boolean(
    device.lastSeenAt &&
    Date.now() - new Date(device.lastSeenAt).getTime() < 600_000,
  );
  const level = device.risks?.[0]?.level ?? "UNKNOWN";
  const activity = history?.entries?.slice(0, 6) ?? [];
  const sensorDataLive =
    valid.soil !== false &&
    valid.environment !== false &&
    latest?.relativeMoisturePct !== null &&
    latest?.temperatureC !== null;
  const loggingActive = Boolean(
    payload.storage?.mounted &&
    (configuration?.configuration?.storage?.retain ?? 0) > 0,
  );
  const deviceUptime = uptime(
    payload.health?.uptime_ms ?? payload.time?.uptime_ms,
  );
  const health = [
    [
      "Soil moisture",
      status(
        valid.soil,
        latest?.relativeMoisturePct !== null &&
          latest?.relativeMoisturePct !== undefined,
      ),
      reading(latest?.relativeMoisturePct, " %", 0),
    ],
    [
      "Air temperature",
      status(
        valid.environment,
        latest?.temperatureC !== null && latest?.temperatureC !== undefined,
      ),
      reading(latest?.temperatureC, " °C"),
    ],
    [
      "Air humidity",
      status(
        valid.environment,
        latest?.humidityPct !== null && latest?.humidityPct !== undefined,
      ),
      reading(latest?.humidityPct, " %", 0),
    ],
    [
      "Battery voltage",
      status(
        valid.battery,
        latest?.batteryV !== null && latest?.batteryV !== undefined,
      ),
      reading(latest?.batteryV, " V", 2),
    ],
    [
      "Solar voltage",
      status(
        valid.solar,
        latest?.solarV !== null && latest?.solarV !== undefined,
      ),
      reading(latest?.solarV, " V", 2),
    ],
  ];
  return (
    <main className="crop-monitor">
      <section className="identity-band">
        <div className="identity-device">
          <Enclosure />
          <div>
            <h1>{device.displayName ?? "Crop Water Stress Monitor"}</h1>
            <p>{device.id}</p>
          </div>
        </div>
        <div className="identity-actions">
          <div className="risk">
            <span>
              {level === "UNKNOWN" ? "Sensor data" : "Water stress risk"}
            </span>
            <strong
              className={
                level === "UNKNOWN"
                  ? sensorDataLive
                    ? "low"
                    : "unknown"
                  : level.toLowerCase()
              }
            >
              {level === "UNKNOWN"
                ? sensorDataLive
                  ? "Live"
                  : "Check sensors"
                : riskLabel(level)}
            </strong>
          </div>
          <div
            className={`connection-badge ${online ? "online" : "offline"}`}
            role="status"
          >
            <i />
            {online ? "Connected" : "Disconnected"}
          </div>
        </div>
      </section>
      <section className="measurements" aria-label="Live device measurements">
        <Metric
          icon="drop"
          label="Relative soil moisture"
          value={reading(latest?.relativeMoisturePct, "%", 0)}
        />
        <Metric
          icon="thermometer"
          label="Air temperature"
          value={reading(latest?.temperatureC, " °C")}
          tone="temperature"
        />
        <Metric
          icon="drop"
          label="Humidity"
          value={reading(latest?.humidityPct, "%", 0)}
          tone="humidity"
        />
        <Metric
          icon="battery"
          label="Battery voltage"
          value={reading(latest?.batteryV, " V", 2)}
          tone="battery"
        />
        <Metric
          icon="sun"
          label="Solar voltage"
          value={reading(latest?.solarV, " V", 2)}
          tone="solar"
        />
        <Metric
          icon="antenna"
          label="Last transmission"
          value={ago(device.lastSeenAt)}
          tone="transmission"
        />
      </section>
      <section className="monitor-content">
        <div className="upper-grid">
          <section className="panel chart-panel">
            <div className="chart-heading">
              <h2>Soil moisture and temperature</h2>
              <div className="series-key">
                <span>
                  <i />
                  Relative soil moisture (%)
                </span>
                <span>
                  <i />
                  Air temperature (°C)
                </span>
              </div>
              <button
                type="button"
                className="history-trigger"
                onClick={() => setHistoryOpen(true)}
              >
                View saved data
              </button>
            </div>
            {chart.length ? (
              <ResponsiveContainer width="100%" height={279}>
                <AreaChart
                  data={chart}
                  margin={{ top: 20, right: 18, left: 8, bottom: 0 }}
                >
                  <defs>
                    <linearGradient
                      id="moisture-area"
                      x1="0"
                      y1="0"
                      x2="0"
                      y2="1"
                    >
                      <stop offset="0" stopColor="#2F7D5B" stopOpacity=".16" />
                      <stop offset="1" stopColor="#2F7D5B" stopOpacity="0" />
                    </linearGradient>
                    <linearGradient
                      id="temperature-area"
                      x1="0"
                      y1="0"
                      x2="0"
                      y2="1"
                    >
                      <stop offset="0" stopColor="#B86645" stopOpacity=".16" />
                      <stop offset="1" stopColor="#B86645" stopOpacity="0" />
                    </linearGradient>
                  </defs>
                  <CartesianGrid stroke="#e5eae8" vertical horizontal />
                  <XAxis
                    dataKey="timestamp"
                    type="number"
                    scale="time"
                    domain={["dataMin", "dataMax"]}
                    tickFormatter={(value) =>
                      new Intl.DateTimeFormat(undefined, {
                        hour: "2-digit",
                        minute: "2-digit",
                        second: "2-digit",
                      }).format(new Date(Number(value)))
                    }
                    tickLine={false}
                    axisLine={{ stroke: "#cfd8d3" }}
                    minTickGap={42}
                  />
                  <YAxis
                    yAxisId="moisture"
                    domain={[0, 100]}
                    tickLine={false}
                    axisLine={false}
                    tick={{ fill: "#2F7D5B" }}
                    tickFormatter={(value) => `${value}%`}
                  />
                  <YAxis
                    yAxisId="temperature"
                    orientation="right"
                    domain={["auto", "auto"]}
                    tickLine={false}
                    axisLine={false}
                    tick={{ fill: "#B86645" }}
                    tickFormatter={(value) => `${value}°`}
                  />
                  <Tooltip
                    labelFormatter={(value) =>
                      new Intl.DateTimeFormat(undefined, {
                        month: "short",
                        day: "numeric",
                        hour: "2-digit",
                        minute: "2-digit",
                        second: "2-digit",
                      }).format(new Date(Number(value)))
                    }
                    cursor={{ stroke: "#95a69c", strokeDasharray: "4 4" }}
                    formatter={(number: number, name: string) => [
                      name === "moisture"
                        ? `${number.toFixed(1)}%`
                        : `${number.toFixed(1)} °C`,
                      name === "moisture"
                        ? "Relative soil moisture"
                        : "Air temperature",
                    ]}
                  />
                  <Area
                    yAxisId="moisture"
                    type="monotone"
                    dataKey="moisture"
                    stroke="#2F7D5B"
                    strokeWidth={2.4}
                    fill="url(#moisture-area)"
                    dot={false}
                    connectNulls={false}
                  />
                  <Area
                    yAxisId="temperature"
                    type="monotone"
                    dataKey="temperature"
                    stroke="#B86645"
                    strokeWidth={2.4}
                    fill="url(#temperature-area)"
                    dot={false}
                    connectNulls={false}
                  />
                </AreaChart>
              </ResponsiveContainer>
            ) : (
              <p className="empty">
                No telemetry has been received for the chart.
              </p>
            )}
          </section>
          <section className="panel health-panel">
            <h2>Sensor health</h2>
            <div className="health-table">
              <div className="health-row table-head">
                <span>Sensor</span>
                <span>Status</span>
                <span>Latest value</span>
                <span>Updated</span>
              </div>
              {health.map(([sensor, state, latestValue]) => (
                <div className="health-row" key={sensor}>
                  <span>{sensor}</span>
                  <span className={state === "OK" ? "ok" : "problem"}>
                    <i />
                    {state}
                  </span>
                  <span>{latestValue}</span>
                  <span>{ago(device.lastSeenAt)}</span>
                </div>
              ))}
            </div>
          </section>
        </div>
        <div className="lower-grid">
          <section className="panel hardware-panel">
            <h2>Hardware overview</h2>
            <div className="hardware-body">
              <Enclosure />
              <dl>
                <div>
                  <dt>Device model</dt>
                  <dd>{device.hardwareModel ?? device.id}</dd>
                </div>
                <div>
                  <dt>Microcontroller</dt>
                  <dd>ESP32-WROOM-32U</dd>
                </div>
                <div>
                  <dt>Power source</dt>
                  <dd>Solar + Battery</dd>
                </div>
                <div>
                  <dt>Sensors</dt>
                  <dd>Soil moisture, Air temperature, Air humidity</dd>
                </div>
                <div>
                  <dt>Last transmission</dt>
                  <dd>{ago(device.lastSeenAt)}</dd>
                </div>
                <div>
                  <dt>Uptime</dt>
                  <dd>{deviceUptime ?? "Awaiting telemetry"}</dd>
                </div>
              </dl>
            </div>
          </section>
          <section className="panel activity-panel">
            <h2>Recent activity</h2>
            <div className="activity-table">
              <div className="activity-row table-head">
                <span>Time</span>
                <span>Event</span>
                <span>Details</span>
              </div>
              {activity.length ? (
                activity.map((item: any) => (
                  <div className="activity-row" key={item.id}>
                    <span>{dateTime(item.at)}</span>
                    <span>
                      {item.type === "telemetry"
                        ? "Data transmission"
                        : rounded(item.title)}
                    </span>
                    <span>{rounded(item.detail)}</span>
                  </div>
                ))
              ) : (
                <p className="empty">No activity has been recorded.</p>
              )}
            </div>
          </section>
          <section className="panel config-panel">
            <h2>Device configuration</h2>
            {command && (
              <p className={`command ${String(command.status).toLowerCase()}`}>
                {command.status === "APPLIED"
                  ? "Configuration confirmed by the device."
                  : (command.message ??
                    `Configuration request ${String(command.status).toLowerCase()}.`)}
              </p>
            )}
            <form onSubmit={save}>
              <label>
                Live telemetry interval
                <select
                  value={draft?.upload_s ?? ""}
                  onChange={(event) =>
                    setDraft((current) =>
                      current
                        ? { ...current, upload_s: Number(event.target.value) }
                        : current,
                    )
                  }
                  disabled={!draft}
                >
                  <option value="5">5 seconds</option>
                  <option value="15">15 seconds</option>
                  <option value="30">30 seconds</option>
                  <option value="60">60 seconds</option>
                </select>
              </label>
              <label>
                Dashboard update check
                <select
                  value={fallbackRefreshSeconds}
                  onChange={(event) =>
                    setFallbackRefreshSeconds(Number(event.target.value))
                  }
                >
                  <option value="1">1 second</option>
                  <option value="5">5 seconds</option>
                  <option value="15">15 seconds</option>
                  <option value="30">30 seconds</option>
                  <option value="60">60 seconds</option>
                </select>
              </label>
              <label className="switch-line">
                Data logging{" "}
                <span className={`switch ${loggingActive ? "on" : "disabled"}`}>
                  <i />
                </span>
                <em>{loggingActive ? "Enabled" : "Storage unavailable"}</em>
              </label>
              <label className="toggle-line">
                Status LED
                <input
                  type="checkbox"
                  checked={draft?.ledEnabled ?? false}
                  disabled={!draft}
                  onChange={(event) =>
                    setDraft((current) =>
                      current
                        ? { ...current, ledEnabled: event.target.checked }
                        : current,
                    )
                  }
                />
                <span />
              </label>
              <label className="toggle-line">
                Buzzer alerts
                <input
                  type="checkbox"
                  checked={draft?.buzzerEnabled ?? false}
                  disabled={!draft}
                  onChange={(event) =>
                    setDraft((current) =>
                      current
                        ? { ...current, buzzerEnabled: event.target.checked }
                        : current,
                    )
                  }
                />
                <span />
              </label>
              <label>
                Stored records
                <span className="setting-value">
                  {payload.storage?.records ?? 0} /{" "}
                  {payload.storage?.capacity_records ?? 0}
                </span>
              </label>
              <button type="submit" disabled={!draft || saving}>
                {saving ? "Sending…" : "Save configuration"}
              </button>
            </form>
          </section>
        </div>
      </section>
      {historyOpen && (
        <div className="history-backdrop" role="presentation">
          <section
            className="history-dialog"
            role="dialog"
            aria-modal="true"
            aria-labelledby="saved-history-title"
          >
            <header className="history-dialog-header">
              <div>
                <p>DATABASE RECORDS</p>
                <h2 id="saved-history-title">Saved sensor readings</h2>
                <span>
                  Choose a period, inspect readings, or download them.
                </span>
              </div>
              <button
                type="button"
                className="history-close"
                onClick={() => setHistoryOpen(false)}
                aria-label="Close saved data"
              >
                ×
              </button>
            </header>
            <form
              className="history-filters"
              onSubmit={(event) => {
                event.preventDefault();
                if (historyRange.from > historyRange.to) {
                  setHistoryError(
                    "The start date must be before the end date.",
                  );
                  return;
                }
                setHistoryError("");
                setHistoryPage(1);
                setHistoryQuery({ ...historyRange });
              }}
            >
              <label>
                From
                <input
                  type="date"
                  value={historyRange.from}
                  max={historyRange.to}
                  onChange={(event) =>
                    setHistoryRange((current) => ({
                      ...current,
                      from: event.target.value,
                    }))
                  }
                />
              </label>
              <label>
                To
                <input
                  type="date"
                  value={historyRange.to}
                  min={historyRange.from}
                  max={dateInputValue(new Date())}
                  onChange={(event) =>
                    setHistoryRange((current) => ({
                      ...current,
                      to: event.target.value,
                    }))
                  }
                />
              </label>
              <button type="submit" className="history-apply">
                Apply range
              </button>
              <button
                type="button"
                className="history-export"
                onClick={downloadHistory}
              >
                Download CSV
              </button>
            </form>
            <div className="history-summary" aria-live="polite">
              {historyLoading
                ? "Loading saved readings…"
                : savedHistory
                  ? `${savedHistory.total.toLocaleString()} readings saved from ${historyQuery.from} to ${historyQuery.to}`
                  : "Saved readings will appear here."}
            </div>
            {historyError && <p className="history-error">{historyError}</p>}
            <div className="history-table-wrap">
              <table className="history-table">
                <thead>
                  <tr>
                    <th>Time</th>
                    <th>Soil moisture</th>
                    <th>Temperature</th>
                    <th>Humidity</th>
                    <th>Battery</th>
                    <th>Solar</th>
                  </tr>
                </thead>
                <tbody>
                  {savedHistory?.records?.length ? (
                    savedHistory.records.map((record: any) => (
                      <tr key={record.id}>
                        <td>
                          {dateTime(record.sampledAt ?? record.ingestedAt)}
                        </td>
                        <td>{reading(record.relativeMoisturePct, "%", 0)}</td>
                        <td>{reading(record.temperatureC, " °C")}</td>
                        <td>{reading(record.humidityPct, "%", 0)}</td>
                        <td>{reading(record.batteryV, " V", 2)}</td>
                        <td>{reading(record.solarV, " V", 2)}</td>
                      </tr>
                    ))
                  ) : (
                    <tr>
                      <td colSpan={6} className="history-empty">
                        {historyLoading
                          ? "Loading readings…"
                          : "No saved readings exist for this period."}
                      </td>
                    </tr>
                  )}
                </tbody>
              </table>
            </div>
            <footer className="history-pagination">
              <span>
                Page {savedHistory?.page ?? 1} of{" "}
                {Math.max(
                  1,
                  Math.ceil(
                    (savedHistory?.total ?? 0) / (savedHistory?.limit ?? 25),
                  ),
                )}
              </span>
              <div>
                <button
                  type="button"
                  disabled={!savedHistory?.hasPrevious || historyLoading}
                  onClick={() =>
                    setHistoryPage((page) => Math.max(1, page - 1))
                  }
                >
                  Previous
                </button>
                <button
                  type="button"
                  disabled={!savedHistory?.hasNext || historyLoading}
                  onClick={() => setHistoryPage((page) => page + 1)}
                >
                  Next
                </button>
              </div>
            </footer>
          </section>
        </div>
      )}
    </main>
  );
}
