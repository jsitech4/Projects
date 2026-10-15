"use client";

import { useEffect, useState } from "react";
import { useRouter } from "next/navigation";

const api = process.env.NEXT_PUBLIC_API_URL ?? "http://localhost:4000";

export default function Devices() {
  const router = useRouter();
  const [message, setMessage] = useState("Opening your monitor…");

  useEffect(() => {
    fetch(`${api}/devices`, { credentials: "include" })
      .then(async (response) => {
        if (!response.ok) throw new Error(response.status === 401 ? "Please sign in to access the monitor." : "The monitor could not be loaded.");
        const devices: { id: string }[] = await response.json();
        if (!devices[0]) throw new Error("No monitor has been registered yet.");
        router.replace(`/devices/${devices[0].id}`);
      })
      .catch((reason: Error) => setMessage(reason.message));
  }, [router]);

  return <main className="device-loading"><span /><p>{message}</p>{message !== "Opening your monitor…" && <a href={message.startsWith("Please") ? "/login" : "/"}>{message.startsWith("Please") ? "Sign in" : "Return home"}</a>}</main>;
}
