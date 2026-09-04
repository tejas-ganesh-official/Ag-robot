#!/usr/bin/env python3
"""
Simple Raspberry Pi camera test script.

Captures a single image from the Pi camera and saves it to disk.
Uses picamera2 (the current standard library on Raspberry Pi OS Bullseye/Bookworm).

Install dependency if needed:
    sudo apt install -y python3-picamera2

Run:
    python3 camera_test.py
"""

import time
from datetime import datetime
from picamera2 import Picamera2


def main():
    # Build a timestamped filename so repeated runs don't overwrite each other
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    filename = f"test_capture_{timestamp}.jpg"

    print("Initializing camera...")
    picam2 = Picamera2()

    # Use a basic still configuration
    config = picam2.create_still_configuration()
    picam2.configure(config)

    picam2.start()
    # Give the camera a moment to adjust exposure/white balance
    time.sleep(2)

    print(f"Capturing image -> {filename}")
    picam2.capture_file(filename)

    picam2.stop()
    print("Done. Image saved as:", filename)


if __name__ == "__main__":
    main()
