# Wi-Fi configuration

`wifi` contains `ssid` (0–32 bytes), `password` (empty for open router networks or 8–63 bytes), `dhcp`, and static `ip`, `gateway`, `subnet`, `dns` strings. The setup AP always uses WPA2; support for an open router network does not make the setup AP open.

Portal scans are asynchronous and return at most 20 SSIDs, RSSI values and security indicators. Hidden SSIDs use manual entry. The ESP32 connects to 2.4 GHz networks; cellular service belongs to the upstream router.

Wi-Fi trials have a 30-second deadline and require successful association/IP assignment before save. Test-only trials restore the previous settings. Failed trials roll back without changing the active NVS revision. Wi-Fi success does not depend on broker availability; the resulting portal state is `wifi_saved_backend_pending_validation`.

Static IPv4 settings require valid octets, a contiguous subnet, usable host addresses and a gateway in the same subnet. The router subnet must not overlap the fixed setup subnet, `192.168.4.0/24`. DHCP routers using that subnet are also rejected during trials. Changing that router's LAN subnet is an installation prerequisite.

STA reconnects use exponential backoff, bounded between practical association waits and five minutes. The firmware does not reboot on router failure. `wifi.disconnect` suspends retries until reconnect or restart; `wifi.forget_network` persists cleared SSID/password after explicit device-ID confirmation. Neither affects queued measurements.

`wifi.configure` and `wifi.test_connection` use `args.settings`; they are asynchronous and return `accepted`, followed by the trial result. Portal blank passwords retain existing credentials; MQTT patches use ordinary explicit JSON semantics, where omission retains and an empty string clears.
