"""
==================================
PYTHON → ARDUINO TIME SENDER DEMO
==================================

WHAT THIS PROGRAM DOES
----------------------
- Opens a serial connection to an Arduino board.
- Every 60 seconds, gets the current time/date/weekday from the computer.
- Formats it as a single line of text:
      "HH:MM,YYYY-MM-DD,WeekdayName\n"
  For example:
      "14:37,2025-12-11,Thursday\n"
- Sends that line to the Arduino over USB.

The Arduino sketch you wrote:
- Listens on Serial.
- Reads one full line at a time.
- Splits the line into time, date, and weekday.
- Updates the TFT display to show those values.

BIG IDEA
--------
The computer (Python) acts as the "time server".
The Arduino acts as a "dumb display" that shows whatever Python says.
"""

# -------- IMPORTS --------
# 'serial' is from the pyserial library, used to talk to the COM port.
# 'time' gives us the sleep() function to wait between sends.
# 'datetime' gives us the current system time in a nice format.
import serial
import time
from datetime import datetime

# -------- SETTINGS --------
# Change this to the COM port where your Arduino is connected.
# On Windows it will be something like "COM3", "COM4", "COM5", etc.
# On macOS/Linux it might be "/dev/ttyUSB0" or "/dev/ttyACM0".
PORT = "COM4"

# This must match Serial.begin(9600) on the Arduino.
BAUD_RATE = 9600

# How many seconds to wait between sending updates.
SEND_INTERVAL_SECONDS = 60


def format_datetime_for_arduino(now: datetime) -> str:
    """
    Take a datetime object (now) and turn it into a string
    in the exact format the Arduino code expects.

    Format: "HH:MM,YYYY-MM-DD,WeekdayName"

    Examples:
        9:05am, 11 December 2025, Thursday
        -> "09:05,2025-12-11,Thursday"
    """
    # %H  = hour (00-23)
    # %M  = minute (00-59)
    # %Y  = year (4 digits)
    # %m  = month (01-12)
    # %d  = day of month (01-31)
    # %A  = full weekday name (Monday, Tuesday, etc.)
    return now.strftime("%H:%M,%Y-%m-%d,%A")


def main():
    """
    Main function:
    - Opens the serial connection.
    - Repeatedly sends the formatted time to the Arduino.
    """

    print("Opening serial connection to Arduino...")

    # Open the serial port.
    # timeout=1 means "wait up to 1 second when reading", but here we only write.
    ser = serial.Serial(PORT, BAUD_RATE, timeout=1)

    # When an Arduino board resets, it often takes a couple of seconds
    # before it is ready to receive data. We wait to be safe.
    print("Waiting for Arduino to reset...")
    time.sleep(2)

    print("Starting to send time updates every", SEND_INTERVAL_SECONDS, "seconds.")
    print("Press Ctrl+C in this window to stop.\n")

    # Infinite loop: keep sending until the user stops the program.
    while True:
        # Get the current system time from the computer.
        now = datetime.now()

        # Format it into the string the Arduino expects.
        payload = format_datetime_for_arduino(now)

        # Add a newline '\n' at the end.
        # The Arduino uses this to know where the message ends.
        message = payload + "\n"

        # Convert the string into bytes and send it over serial.
        ser.write(message.encode("utf-8"))

        # Print to the terminal so we can see what was sent.
        print("Sent to Arduino:", payload)

        # Wait before sending the next update.
        time.sleep(SEND_INTERVAL_SECONDS)


# This is the standard Python pattern to say:
# "Only run main() if this file is executed directly,
#  not if it is imported as a module."
if __name__ == "__main__":
    # Wrap main() in a try/except so we can close the port cleanly on Ctrl+C.
    try:
        main()
    except KeyboardInterrupt:
        # This runs when the user presses Ctrl+C to stop the script.
        print("\nStopped by user.")
    except serial.SerialException as e:
        # This runs if there is a problem opening or using the COM port.
        print("Serial error:", e)
    except Exception as e:
        # Catch-all for any other unexpected errors.
        print("Unexpected error:", e)
