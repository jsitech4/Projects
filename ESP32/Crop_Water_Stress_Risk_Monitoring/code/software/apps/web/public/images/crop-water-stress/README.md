# Crop Water Stress Monitor images

This folder contains the artwork shown in the Crop Water Stress Monitor web app.

- `header-enclosure.svg` is used only in the device header.
- `overview-enclosure.svg` is used only in the **Hardware overview** card.

Replace either SVG independently to change that area only. Keeping the `viewBox` close to `0 0 150 238` preserves the current layout. A PNG or WebP can also be used, but change its matching `.svg` extension in `app/devices/[id]/page.tsx` to match the new file name.
