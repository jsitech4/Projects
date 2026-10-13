# Deployment and service

Release deployment requires a commissioned per-device firmware image, a verified 4 MiB ESP32 module, validated analog profiles and a broker account/CA. No OTA partition or update service is enabled; `firmware.get_update_status` reports serial upload as the update method. `firmware.check_update` and `firmware.start_verified_update` are not advertised and return unsupported.

Build/upload/monitor:

```powershell
$env:CWS_FACTORY_FILE = (Resolve-Path provisioning/private/<unit>/factory.json).Path
pio run -e esp32_wroom_32u
pio device list
pio run -e esp32_wroom_32u -t upload
pio device monitor -b 115200
```

Identify the actual port before an optional `--upload-port` override. Ordinary firmware upload preserves NVS and LittleFS with this unchanged partition table. Do not erase flash or upload a filesystem image during routine updates. A changed partition layout requires a separate backup/migration service plan.

Before release, record physical flash ID/capacity; GPIO2 boot loading; buzzer driver type/polarity/current; sensor supply/data levels; LCD level shifting/backpack mapping; encoder population; independent divider values and unclipped ranges; antenna/radio operation; verified broker CA/hostname; clock availability; command ACLs; dynamic heap/stack at TLS peak; and brownout behavior.

Acceptance includes headless cold boot, optional-peripheral absence, first-password replacement/reboot, old-password rejection, five-minute setup expiry, valid/invalid connection replacements, offline acquisition, retention/overflow, duplicate command suppression and durable backend synchronization. Preserve the test evidence in the unit's private commissioning record.

Forgotten-password recovery uses the per-device service token and physical serial access; it does not erase records. Storage initialization is a separate explicitly destructive factory/service action. Images and owner/service records contain sensitive material and remain outside version control.
