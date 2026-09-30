# Midnight / Amber display design

Keep aircraft dominant over subdued geography. All pages share logical RGB palette values in `include/ui/display_theme.h`; panel channel order/inversion are configured separately. Use navy background/land/water, cyan navigation and fresh vectors, amber aircraft/headline metrics, light primary text and muted captions/held targets. Use red with text for explicit unhealthy states.

Project map and aircraft from the same saved center/range. Clip geography to the 107-pixel circle. Use real bundled world polygons; do not add decorative map shapes. Streets and small geographic details are unavailable. Keep one shared RGB565 framebuffer and bounded geometry cache.

Only the 25 km page has aircraft callsign/type/altitude text. At 50/100 km, keep symbols and fresh vectors without tags. Preserve airport/compass/range labels. Held positions are muted at 15 seconds, omit vectors and expire by 30 seconds.

Rotate 25/50/100 km Radar then Station every 25 seconds. Single-click advances pages; double-click advances ranges. Page changes are RAM-only, preserving held positions and the normal request cadence. Scale labels use the third ring; outer radius is 4/3 of the label.

Station uses a consistent ten-second snapshot with explicit ONLINE/DELAYED/OFFLINE and unavailable metric dashes. Zero aircraft is healthy when the feed advances. Pi temperature is cyan below 60 C, amber at 60–80 C inclusive and red above 80 C. Keep all text within the round viewport.

Validate native logic, actual framebuffer alignment and runtime resources separately from physical panel colors, button behavior and cold power cycles. Public screenshots must use synthetic/public example data rather than a user's live receiver data.
