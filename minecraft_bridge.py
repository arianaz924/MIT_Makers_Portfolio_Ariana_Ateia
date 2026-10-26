import os
import time
from pathlib import Path
import serial

arduino = serial.Serial("COM3",9600,timeout=1)
time.sleep(2)

log_path = Path(os.environ["APPDATA"]) / ".minecraft" / "logs" / "latest.log"

print("Watching Minecraft...")

with open(log_path,"r",encoding="utf-8",errors="ignore") as log:
    log.seek(0,2)

    while True:
        line = log.readline()

        if not line:
            time.sleep(0.05)
            continue

        for cmd in [
            "LED1_ON","LED1_OFF",
            "LED2_ON","LED2_OFF",
            "LED3_ON","LED3_OFF",
            "LED4_ON","LED4_OFF"
        ]:
            if cmd in line:
                arduino.write((cmd+"\n").encode())
                print("Sent",cmd)