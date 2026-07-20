# web2osc — Session Handoff

Bridges the **TELSP-2501 SEL-3505 RTAC** REST API to **OSC**, running on an
**Adafruit Feather ESP32-S3 TFT**. Polls RTAC data tags over HTTP(S) and
forwards each value out as an OSC message.

## Status: proof-of-concept complete ✅

The full chain is validated end-to-end on real hardware, against a local mock
of the RTAC (the physical RTAC is not yet reachable off-site):

```
Feather ESP32-S3  →  Wi-Fi  →  GET RTAC (Basic Auth)  →  parse "instMag"  →  OSC  →  Protokol
```

- API contract, Basic Auth, URL format, and `instMag` extraction confirmed via `curl` + the mock.
- Firmware compiles and runs on the actual S3; OSC messages (`/rtac/<tag>`) confirmed landing in Protokol.

## How the API works (from the RTAC Implementation Manual R1, §8.2)

- **Endpoint (GET):** `https://<rtac-ip>/api/v1/logic-engine/symbols/Tags.<TagName>`
- **Auth:** HTTP Basic (credentials live in `config.h` and the manual — *not* in this repo).
- **Response:** JSON; the current value is under the key **`instMag`**.
- **Tags:** the Table 1 data map (grid/solar power, daily/monthly energy, GHG offsets, etc.).

## Files

| File | Purpose | In git? |
|---|---|---|
| `web2osc.ino` | Firmware: poll tag list → parse `instMag` → OSC out | ✅ |
| `config.h` | Wi-Fi creds, `rtacBase`/`rtacUser`/`rtacPass`, OSC target | ❌ gitignored (secrets) |
| `mock_rtac.py` | Local RTAC emulator for off-site dev (port 8080) | ✅ |
| `scratch_sim.py` | Software mirror of the firmware poll loop | ✅ |
| `osc_listener.py` | Simple Python OSC sink (Protokol preferred for real testing) | ✅ |
| `*.pdf` (Aurora spec, RTAC manual) | Reference docs | ❌ gitignored (proprietary + credentials) |

## Dev / test loop (no RTAC needed)

1. `python3 mock_rtac.py` — starts the mock on `:8080` (auth + tags per the manual).
2. Set `config.h` `rtacBase` to this Mac's LAN IP, e.g. `http://192.168.4.x:8080`.
3. Flash: `arduino-cli compile --fqbn esp32:esp32:adafruit_feather_esp32s3_tft --upload -p <port> .`
4. Watch OSC in Protokol (UDP 8000).

Toolchain: `arduino-cli` + core `esp32:esp32` 3.3.10; libs `ArduinoJson` 7, `OSC` (CNMAT).

## TODO on-site (before trusting the live RTAC)

1. **Switch to the real device** — set `rtacBase` to `https://<rtac-ip>` in `config.h`.
2. **Verify JSON nesting** — the manual only guarantees the value is under `instMag`
   but shows the structure as a photo. If it's nested (e.g. `value.instMag`),
   adjust the one parse line in `web2osc.ino`.
3. **Lock down HTTPS** — the code currently uses `client.setInsecure()`. Pin the
   device's real certificate/fingerprint before production.
4. **Tag name check** — the manual is inconsistent: Table 1 says `Trees_Equivalency`
   but the logic writes `Tree_Equivalency`. Confirm the actual tag on the box.
5. Optionally expand the polled `tags[]` list in `web2osc.ino` to the full Table 1.
