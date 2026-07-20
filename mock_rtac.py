#!/usr/bin/env python3
"""
Mock of the TELSP-2501 SEL-3505 RTAC REST API, per the RTAC Implementation
Manual R1 (section 8.2). Lets us develop/test web2osc without the real device.

Emulates:
  - HTTP Basic Auth  (user: api  /  pass: limbic)
  - GET /api/v1/logic-engine/symbols/Tags.<TagName>
  - JSON response whose current value lives under the key "instMag"

Run:   python3 mock_rtac.py           (serves on http://0.0.0.0:8080)
Test:  curl -u api:limbic http://localhost:8080/api/v1/logic-engine/symbols/Tags.Power_Consumption_Solar

NOTE: the exact JSON *nesting* the real RTAC returns isn't in the manual text
(it references a photo). The manual only guarantees the value is under
"instMag". This mock returns instMag at the top level plus typical SEL quality/
timestamp fields. If the live device nests it deeper, we adjust the parser then.
"""
import base64
import json
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

USERNAME = "api"
PASSWORD = "limbic"
PORT = 8080

# Table 1 tags with plausible live values (units per the manual).
TAGS = {
    "Power_Consumption_Grid":   12.4,    # kW
    "Power_Consumption_Solar":  48.7,    # kW
    "Power_Ratio":              25,      # %
    "Energy_Solar_Today":       310,     # kWh
    "Energy_Solar_Yesterday":   402,     # kWh
    "Energy_Solar_Month":       9125,    # kWh
    "Energy_Solar_Last_Month":  11840,   # kWh
    "Energy_Grid_Today":        88,      # kWh
    "Energy_Grid_Yesterday":    140,     # kWh
    "Energy_Grid_Month":        2670,    # kWh
    "Energy_Grid_Last_Month":   3410,    # kWh
    "Power_Percent_Solar":      79,      # %
    "Capacity_Factor":          34,      # %
    "GHG_Prevented":            14.6,    # Metric Tonnes
    "Vehicle_Equivalency":      3,       # #
    "Trees_Equivalency":        670,     # #
}

PREFIX = "/api/v1/logic-engine/symbols/Tags."


class Handler(BaseHTTPRequestHandler):
    def _unauthorized(self):
        self.send_response(401)
        self.send_header("WWW-Authenticate", 'Basic realm="RTAC"')
        self.end_headers()

    def _authed(self):
        header = self.headers.get("Authorization", "")
        if not header.startswith("Basic "):
            return False
        try:
            decoded = base64.b64decode(header[6:]).decode()
        except Exception:
            return False
        return decoded == f"{USERNAME}:{PASSWORD}"

    def do_GET(self):
        if not self._authed():
            self._unauthorized()
            return

        if not self.path.startswith(PREFIX):
            self.send_response(404)
            self.end_headers()
            return

        tag = self.path[len(PREFIX):]
        if tag not in TAGS:
            self.send_response(404)
            self.end_headers()
            self.wfile.write(b'{"error":"unknown tag"}')
            return

        body = {
            "name": f"Tags.{tag}",
            "instMag": TAGS[tag],
            "q": {"validity": "good"},
            "t": "2026-07-20T12:00:00Z",
        }
        payload = json.dumps(body).encode()
        self.send_response(200)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(payload)))
        self.end_headers()
        self.wfile.write(payload)

    def log_message(self, fmt, *args):
        print("  [mock-rtac]", self.address_string(), fmt % args)


if __name__ == "__main__":
    print(f"Mock RTAC API on http://0.0.0.0:{PORT}{PREFIX}<TagName>")
    print(f"Auth: {USERNAME}/{PASSWORD}   Tags: {len(TAGS)}")
    ThreadingHTTPServer(("0.0.0.0", PORT), Handler).serve_forever()
