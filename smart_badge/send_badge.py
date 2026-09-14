#!/usr/bin/env python3
"""
CLI Badge Updater for LilyGo TTGO T5 V2.3.1 E-Paper Smart Badge
Communicates directly via USB Serial (/dev/ttyUSB0) at 115200 baud.
"""

import argparse
import json
import sys
import time
import serial
import serial.tools.list_ports

def find_esp32_port():
    ports = list(serial.tools.list_ports.comports())
    for p in ports:
        if "USB" in p.device or "ACM" in p.device or "CP210" in (p.description or "") or "CH340" in (p.description or ""):
            return p.device
    return "/dev/ttyUSB0"

def main():
    parser = argparse.ArgumentParser(description="Update E-Paper Badge directly over USB Serial")
    parser.add_argument("--port", default=None, help="Serial port (e.g. /dev/ttyUSB0)")
    parser.add_argument("--baud", type=int, default=115200, help="Baud rate (default: 115200)")
    parser.add_argument("--mode", choices=["conf", "lugg", "stat"], default="conf",
                        help="Badge mode: conf (Conference), lugg (Luggage), stat (Desk Status)")
    
    # Conference fields
    parser.add_argument("--name", default="Alex Rivers", help="Attendee / Owner name")
    parser.add_argument("--title", default="Software Engineer", help="Job title / role")
    parser.add_argument("--company", default="Acme Corp", help="Company / Org")
    parser.add_argument("--handle", default="@alexrivers", help="Social handle")
    parser.add_argument("--qr", default="https://github.com", help="QR Code URL / Text")

    # Luggage fields
    parser.add_argument("--phone", default="+1 (555) 019-2834", help="Luggage contact phone")
    parser.add_argument("--email", default="alex@example.com", help="Luggage contact email")
    parser.add_argument("--note", default="Reward if returned intact!", help="Luggage note")

    # Desk Status fields
    parser.add_argument("--stat-title", default="DO NOT DISTURB", help="Notice title")
    parser.add_argument("--stat-sub", default="Deep Focus / In A Meeting", help="Notice subtitle")
    parser.add_argument("--stat-foot", default="Back online at 2:00 PM", help="Notice footer")

    args = parser.parse_args()

    mode_map = {"conf": 0, "lugg": 1, "stat": 2}
    active_mode = mode_map[args.mode]

    payload = {
        "mode": active_mode,
        "name": args.name,
        "title": args.title,
        "company": args.company,
        "handle": args.handle,
        "qr": args.qr,
        "owner": args.name,
        "phone": args.phone,
        "email": args.email,
        "note": args.note,
        "stitle": args.stat_title,
        "subtitle": args.stat_sub,
        "footer": args.stat_foot
    }

    port = args.port or find_esp32_port()
    json_str = json.dumps(payload)

    print("=" * 60)
    print(f" Connecting to E-Paper Badge on {port}...")
    print(f" Mode: {args.mode.upper()}")
    print("=" * 60)

    try:
        # Note: Do not toggle DTR/RTS aggressively to avoid unwanted resets if in pairing loop
        ser = serial.Serial(port, args.baud, timeout=2.0)
        time.sleep(0.5)

        # Transmit JSON line
        ser.write((json_str + "\n").encode("utf-8"))
        ser.flush()
        print(f" Transmitted payload: {json_str}")

        # Read responses for up to 3 seconds
        start_t = time.time()
        updated = False
        while time.time() - start_t < 3.0:
            line = ser.readline().decode("utf-8", errors="ignore").strip()
            if line:
                print(f" [Badge] {line}")
                if "RESPONSE:" in line or "ACK:OK" in line or "success" in line:
                    updated = True
                    break

        ser.close()
        if updated:
            print("\n🎉 Badge updated successfully! E-paper display is refreshing.")
        else:
            print("\n⚠️ Payload sent. If the device was in deep sleep, hold the onboard button for 1.5s to enter Pairing Mode and rerun.")
    except Exception as e:
        print(f"\n❌ Serial error: {e}", file=sys.stderr)
        sys.exit(1)

if __name__ == "__main__":
    main()
