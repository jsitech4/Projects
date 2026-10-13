"use client";
import { useEffect, useState } from "react";
const api = process.env.NEXT_PUBLIC_API_URL ?? "http://localhost:4000";
type Device = {
  id: string;
  displayName?: string;
  lastSeenAt?: string;
  risks: { level: string; explanation: string }[];
  plot?: { name: string; site: { name: string } };
};
export default function Devices() {
  const [devices, setDevices] = useState<Device[]>([]);
  const [error, setError] = useState("");
  useEffect(() => {
    fetch(`${api}/devices`, { credentials: "include" })
      .then(async (r) =>
        r.ok
          ? setDevices(await r.json())
          : setError("Sign in to view registered devices."),
      )
      .catch(() => setError("The API is unavailable."));
  }, []);
  return (
    <section className="fleet-page">
      <a className="fleet-brand" href="/">🌿 Farm Water Monitor</a>
      <p className="eyebrow">Farm operations</p>
      <h1>Monitored fields</h1>
      <p className="fleet-subtitle">Select a field to review current conditions, recorded readings and device status.</p>
      {error && <p className="error">{error}</p>}
      <div className="fleet-grid">
        {devices.map((d) => (
          <a className="field-card" href={`/devices/${d.id}`} key={d.id}>
            <h2>{d.displayName || d.id}</h2>
            <p>
              {d.plot
                ? `${d.plot.site.name} / ${d.plot.name}`
                : "Plot assignment pending"}
            </p>
            <p>
              Last seen:{" "}
              {d.lastSeenAt ? new Date(d.lastSeenAt).toLocaleString() : "Never"}
            </p>
            <span className={`risk ${d.risks[0]?.level ?? "UNKNOWN"}`}>
              {d.risks[0]?.level ?? "UNKNOWN"}
            </span>
            <p>{d.risks[0]?.explanation ?? "No current water-stress assessment is available."}</p>
          </a>
        ))}
      </div>
    </section>
  );
}
