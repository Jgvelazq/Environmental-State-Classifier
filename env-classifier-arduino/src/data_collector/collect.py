"""
collect.py -- captures sensor readings from the Arduino over serial and
appends them to a labeled CSV file.

Usage:
    python collect.py <serial_port> <label> [--seconds N]

Example session (run once per condition you want to capture):
    python collect.py /dev/ttyACM0 bright_warm --seconds 30
    python collect.py /dev/ttyACM0 bright_cool --seconds 30
    python collect.py /dev/ttyACM0 dark_warm --seconds 30
    python collect.py /dev/ttyACM0 dark_cool --seconds 30

Each run appends rows to dataset.csv with the given label attached, so you
end up with one combined, labeled dataset after running all four sessions
(ideally several times each, across different days/times, for variety).

Finding your serial port:
    - Windows: check Device Manager -> Ports (COM3, COM4, etc.)
    - Mac: usually /dev/cu.usbmodemXXXX or /dev/cu.usbserial-XXXX
    - Linux: usually /dev/ttyACM0 or /dev/ttyUSB0
"""

import argparse
import csv
import sys
import time
from pathlib import Path

import serial

DATASET_PATH = Path(__file__).parent / "dataset.csv"
EXPECTED_HEADER = "raw_thermistor,raw_photoresistor,dht_temp_c,dht_humidity"


def main():
    parser = argparse.ArgumentParser(description="Collect labeled sensor data from Arduino over serial.")
    parser.add_argument("port", help="Serial port, e.g. /dev/ttyACM0 or COM3")
    parser.add_argument("label", help="Label to attach to every row collected this session, e.g. bright_warm")
    parser.add_argument("--baud", type=int, default=9600, help="Baud rate (default 9600, must match the sketch)")
    parser.add_argument("--seconds", type=float, default=30, help="How long to collect for (default 30s)")
    args = parser.parse_args()

    print(f"Opening {args.port} at {args.baud} baud...")
    try:
        ser = serial.Serial(args.port, args.baud, timeout=2)
    except serial.SerialException as e:
        print(f"Could not open serial port: {e}")
        print("Check that the Arduino is plugged in, the port name is correct, "
              "and the Arduino IDE's Serial Monitor is closed (it locks the port).")
        sys.exit(1)

    # Arduino resets when the serial connection opens; give it a moment
    # to boot and start sending data, and skip the header line it prints.
    time.sleep(2)
    ser.reset_input_buffer()

    file_exists = DATASET_PATH.exists()
    rows_collected = 0
    start = time.time()

    print(f"Collecting for {args.seconds}s with label '{args.label}'. "
          f"Go create that condition now (cup your hand over the photoresistor, "
          f"breathe on the thermistor, etc.)...")

    with open(DATASET_PATH, "a", newline="") as f:
        writer = csv.writer(f)
        if not file_exists:
            writer.writerow(["raw_thermistor", "raw_photoresistor", "dht_temp_c", "dht_humidity", "label"])

        while time.time() - start < args.seconds:
            line = ser.readline().decode("utf-8", errors="ignore").strip()
            if not line or line == EXPECTED_HEADER:
                continue  # skip blank lines and the sketch's own header line

            parts = line.split(",")
            if len(parts) != 4:
                print(f"Skipping malformed line: {line!r}")
                continue

            try:
                thermistor_raw = int(parts[0])
                photo_raw = int(parts[1])
                dht_temp = float(parts[2])
                dht_humidity = float(parts[3])
            except ValueError:
                print(f"Skipping unparseable line: {line!r}")
                continue

            writer.writerow([thermistor_raw, photo_raw, dht_temp, dht_humidity, args.label])
            rows_collected += 1
            print(f"\r  {rows_collected} rows collected...", end="", flush=True)

    ser.close()
    print(f"\nDone. {rows_collected} rows appended to {DATASET_PATH} with label '{args.label}'.")


if __name__ == "__main__":
    main()
