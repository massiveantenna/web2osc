#!/usr/bin/env python3
"""
Software mirror of web2osc.ino's poll loop, to prove the chain end-to-end
without an ESP32. Same tags, same Basic Auth, same instMag parse, real OSC out.
"""
import base64, json, urllib.request
from pythonosc.udp_client import SimpleUDPClient

RTAC_BASE = "http://localhost:8080"          # mock RTAC
RTAC_USER, RTAC_PASS = "api", "limbic"
OSC_IP, OSC_PORT = "127.0.0.1", 8000

TAGS = [
    "Power_Consumption_Grid", "Power_Consumption_Solar", "Power_Ratio",
    "Energy_Solar_Today", "Energy_Grid_Today", "Power_Percent_Solar",
    "GHG_Prevented", "Vehicle_Equivalency", "Trees_Equivalency",
]

osc = SimpleUDPClient(OSC_IP, OSC_PORT)
auth = base64.b64encode(f"{RTAC_USER}:{RTAC_PASS}".encode()).decode()

print("Polling RTAC:")
ok = 0
for tag in TAGS:
    url = f"{RTAC_BASE}/api/v1/logic-engine/symbols/Tags.{tag}"
    req = urllib.request.Request(url, headers={"Authorization": f"Basic {auth}"})
    try:
        with urllib.request.urlopen(req, timeout=3) as r:
            doc = json.load(r)
    except Exception as e:
        print(f"  {tag} -> HTTP error {e}")
        continue
    if "instMag" not in doc:
        print(f"  {tag} -> no 'instMag'")
        continue
    value = float(doc["instMag"])
    osc.send_message(f"/rtac/{tag}", value)
    print(f"  {tag} = {value:.3f}  -> OSC /rtac/{tag}")
    ok += 1
print(f"Sweep done: {ok}/{len(TAGS)} tags OK")
