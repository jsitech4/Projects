# Licensing guide for the project collection

This collection uses a separate standard license for each type of original
work. The project-level notices—not a folder name alone—define the material
covered by each license. A project README links to its notices.

| Material                                                                                                                                 | License         | Project file             |
| ---------------------------------------------------------------------------------------------------------------------------------------- | --------------- | ------------------------ |
| Firmware, software, scripts, configuration examples, tests, and web applications                                                         | MIT             | `LICENSE`                |
| Original electronic hardware design: KiCad sources, original symbols/footprints/3D design assets, BOMs, and original fabrication exports | CERN-OHL-P-2.0  | `LICENSE-HARDWARE`       |
| Original written documentation, diagrams, photographs, screenshots, and README text                                                      | CC BY 4.0       | `LICENSE-DOCUMENTATION`  |
| Vendor or otherwise external files, dependencies, logos, and trademarks                                                                  | Their own terms | `THIRD-PARTY-NOTICES.md` |

The Crop Water-Stress platform currently has no top-level KiCad or production
hardware design folder, so it has no `LICENSE-HARDWARE` file. Its ESP32
firmware is software and therefore remains under MIT.

## What the split means

The existing, unmodified MIT `LICENSE` remains the license for original code.
It does not license the original PCB/hardware design or documentation after
this split. Hardware design files are expressly made available under the
permissive CERN Open Hardware Licence v2, which is written to cover design
source and products made from it. Original documentation uses CC BY 4.0, which
permits reuse and adaptation subject to attribution.

The hardware license is permissive: a downstream user may modify and convey
the original hardware design or make and convey products, while retaining the
required notices. It does not grant trademark rights or change a component
supplier's terms. The documentation license requires appropriate attribution
and an indication of changes when shared.

## External content and practical limits

Datasheets, vendor KiCad libraries/models, PlatformIO/Arduino/Python/npm
packages, Docker images, logos, and other non-original material are excluded
unless an embedded notice says otherwise. Keep their notices and check their
licenses independently. The licenses also do not certify a design, grant
regulatory approval, remove privacy obligations, or transfer rights that JSI
Tech does not own.

For the canonical texts, see [LICENSES/README.md](LICENSES/README.md). For a
commercial release, regulated product, or meaningful IP risk, have the final
set of notices reviewed by a qualified lawyer in the deployment jurisdiction.
