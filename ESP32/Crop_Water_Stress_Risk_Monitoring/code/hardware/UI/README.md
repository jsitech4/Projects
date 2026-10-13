# Local admin interface preview

Open **test.html** directly in Chrome, Edge or Firefox. No server, ESP32, internet connection or package installation is required.

The default client view shows only the production administration interface, with no preview banner or simulation controls. Sign in as `admin` using **Preview-admin!42**. These are public preview credentials only.

For engineering review, append `?review=1` to the file URL (for example, `test.html?review=1`). This reveals the amber controls, including **Fill demo password**. They select a configured, first-commissioning or offline device; control success/failure of connection trials; expire the setup window; and reset the preview. The production countdown runs for five minutes. The review parameter changes presentation only; the standalone page always uses simulated responses.

All production sections are present: overview, Wi-Fi, MQTT/TLS, security, calibration and diagnostics. Calibration is shown only when the privileged password is used at the ordinary sign-in screen; an administrator sign-in never sees it. Scanning, connection tests, saving, cancellation, password replacement, calibration and logout use in-memory simulated responses. Test-only trials restore previous settings. Failed trials do not save changes. A successful password replacement rejects the old preview password until reset. The first-commissioning scenario requires password replacement before showing configuration pages.

Use dummy credentials only. No submissions leave the page, and there is no persistent browser storage. Reloading or changing scenario resets all settings and the password. Certificate imports receive only basic format checks in the preview; hardware-side TLS validation and electrical behavior are outside its scope. Device names, diagnostics and connection results are synthetic even where the unchanged production wording refers to live state.

## Keeping the preview aligned

The HTML, CSS and application JavaScript come from the real firmware files under `data/portal/`. Edit those files to change the interface that will ship on the ESP32, then refresh the standalone preview:

```powershell
python tools/build_preview.py
```

`UI/test.html` is the generated, portable preview. `UI/controls.html` and `UI/preview.js` supply only the review controls and simulated API. They are not embedded into the firmware. Changes made directly to `test.html` are overwritten when it is rebuilt.
