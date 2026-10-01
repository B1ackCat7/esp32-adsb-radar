# Upstream synchronization

The `adsb-station` branch integrates upstream `main` through
[`139a3ac7d173d0f1a03ac66d121e2dc8aba35718`](https://github.com/MatixYo/ESP32-Plane-Radar/commit/139a3ac7d173d0f1a03ac66d121e2dc8aba35718).
The original public beta started at v1.1.4 (`9d857787`); ten subsequent
upstream commits, including merge commits, were reviewed for this update.
This is a merge of the upstream ancestry with explicit compatibility
resolutions. Future changes still need review.

| Upstream change | Local receiver decision |
| --- | --- |
| Filtered streaming JSON | Already present in the local receiver client; keep its filters, 200,000-byte wire limit, 1.5-second idle timeout and six-second body deadline. |
| HTTP/1.1 chunk decoding | Adopt the upstream `BodyFramer` and use it for all three station endpoints. Support chunked, content-length and close-delimited JSON. Drain/check known framing before publishing data, including a complete JSON followed by a damaged chunk terminator. Reject unsupported transfer/content encodings. |
| Refresh settings after save | Already covered by our redirect to a freshly rendered form with current saved values and revision checks. Preserve the custom settings routes. |
| Background HTTPS worker / five-second fetching | Keep the local ten-second phased scheduler and cooperative body reader. A second task would need synchronization across settings, aircraft retention and Station metrics. The local reader serves the portal/buttons while waiting for response bytes. Connect/header operations can still block for their bounded timeouts. |
| Estimated aircraft movement / four-Hz redraw | Keep measured positions and existing held-position semantics. Estimating motion changes the meaning of positions during a delayed feed and is a separate feature decision. |
| RGB332 framebuffer | Keep the shared 240×240 RGB565 buffer (115,200 pixel bytes), preserving the Midnight/Amber palette. Upstream's 8-bit buffer would save 57,600 pixel bytes, but quantizes these colors and needs separate visual/physical acceptance. No second framebuffer is introduced. |

The local receiver IP/ports, blank first-install fields, neutral internal
coordinates, on-device world-map generation, page controls and feed-health
timings remain unchanged. This merge does not add the external adsb.fi API.

GitHub should show zero commits behind this reviewed upstream commit after
publication. Commits ahead are expected: they implement this fork's local
station features and compatibility decisions. This does not imply that every
upstream feature is enabled.
