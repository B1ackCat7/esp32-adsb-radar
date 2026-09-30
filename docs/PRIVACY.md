# Privacy and public distribution

The firmware contains no preconfigured station, Wi-Fi credentials or household coordinates. New station/center fields are blank. Generic examples are `192.168.1.100` and `0, 0`; the setup AP uses `192.168.4.1`. World map and airport coordinates are public geographic datasets.

Users enter their own settings through the local portal. Settings and Wi-Fi credentials persist on the device in NVS. Runtime IPs and coordinates are shown only where needed for local configuration; connected SSIDs are not printed in the firmware's serial success messages. There is no installation telemetry, cloud aircraft fallback or external map request. Firmware checks its configured receiver using local HTTP. The portal is intended for a trusted network and has no authentication.

Public source is prepared independently from a public upstream checkout. Private development commits, operational handoffs, device backups, settings exports, live traffic captures and local logs are not included. New maintainer commits use a GitHub noreply address. Build scripts normalize project/build-cache paths before compiling, and release images are generated from source rather than read from a device. Merged-image NVS padding is blank.

Before publishing changes, run `python3 scripts/privacy_check.py`, native tests, and a clean firmware build. Also inspect staged changes and binary strings for your own network, location, credentials and machine paths; automated patterns cannot establish the absence of every possible secret. Keep device dumps, real-data screenshots and private audit evidence outside this checkout. The ignore rules are a safeguard, not a substitute for reviewing staged content.
