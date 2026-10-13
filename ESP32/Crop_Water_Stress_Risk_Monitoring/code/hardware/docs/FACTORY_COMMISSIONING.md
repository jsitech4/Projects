# Factory commissioning

1. Inspect the manufactured PCB against [Hardware](HARDWARE.md), including flash capacity, divider values, voltage levels and output drivers.
2. Install PlatformIO and `tools/requirements.txt` in a local Python environment.
3. Run `python tools/commission.py create --directory provisioning/private/<unit>`. Enter and confirm a strong initial administrator password. The tool creates unique setup-network credentials and service material without printing them.
4. Set `CWS_FACTORY_FILE` to that unit's absolute `factory.json` path, then build and upload the firmware using the README commands.
5. Verify the device's stable ID, associate it with the unit's commissioning record and label its setup SSID/password. Deliver only `owner.json` to the owner. Protect `service.json` separately.
6. Initialize blank LittleFS storage through the service interface:

```powershell
.\.venv\Scripts\python.exe tools/commission.py service --port <identified-port> --record provisioning/private/<unit>/service.json --operation initialize_storage
```

The tool waits for a restart, then sends the service token during the first 60 seconds. This operation formats the filesystem and is intended for blank factory units. Mounted storage containing records rejects initialization. An unmountable/corrupt filesystem requires a deliberate service decision before this operation because formatting destroys its contents.

7. Restart, authenticate to the setup portal, replace the initial administrator password, test/save Wi-Fi, import the broker CA and test/save MQTT.
8. Commission calibration through the local Developer calibration page after administrator and developer authentication. Record the measurements, calibration revision, firmware hash and acceptance results.

`factory.json` contains the setup password, 16-byte administrator salt, PBKDF2-HMAC-SHA256 verifier and SHA-256 service-token verifier. Generated secret headers and private directories are ignored. A firmware image compiled with this material is also a private artifact. The build does not package the owner record or service token.

No production credentials are checked in. A build without `CWS_FACTORY_FILE` remains uncommissioned unless a valid replacement administrator verifier already exists; a setup-network credential is still required. The project does not enable flash encryption or secure boot.
