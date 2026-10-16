# Crop Water Stress Monitor images

This folder holds raster images for the live monitor.

- `overview-enclosure.png` appears only in the **Hardware overview** card.
- `header-enclosure.png` is not currently used; it is retained in case you want to reintroduce a header image later.

Replace `overview-enclosure.png` to change the Hardware overview image. A portrait-oriented PNG with a transparent background works best. JPG is also supported: rename the chosen file to `.jpg`, then change its `.png` extension in `app/devices/[id]/page.tsx`.
