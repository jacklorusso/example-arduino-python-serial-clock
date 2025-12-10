/*
   ===============================
   ARDUINO TFT CLOCK DISPLAY DEMO
   ===============================

   WHAT THIS PROGRAM DOES
   ----------------------
   - Listens on the USB serial connection for a line of text sent from a Python script.
   - The Python script sends:
        "HH:MM,YYYY-MM-DD,WeekdayName\n"
     e.g. "14:37,2025-12-11,Thursday"
   - The Arduino:
        1. Reads that line from Serial.
        2. Splits it into time, date, and weekday.
        3. Stores these in a struct called nowInfo.
        4. Redraws the TFT screen to show the new time/date/weekday.

   BIG IDEAS
   ---------
   - The Arduino is acting like a "dumb screen" that trusts the computer
     to provide the correct time.
   - We only redraw the screen when NEW data arrives (to avoid flicker
     and unnecessary drawing).
*/

// =================== LIBRARIES ===================
// These libraries know how to talk to the TFT screen and draw text/shapes.

#include <Adafruit_GFX.h>   // Generic graphics functions (draw text, lines, etc.)
#include <MCUFRIEND_kbv.h>  // Driver for MCUFRIEND TFT shields
MCUFRIEND_kbv tft;          // Create a TFT object we can use to control the screen

// Font libraries so we can use nicer, scalable fonts.
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSans12pt7b.h>
#include <Fonts/FreeSerif12pt7b.h>
#include <FreeDefaultFonts.h>

// =================== COLOUR DEFINITIONS ===================
// These are 16-bit colour codes (RGB565 format).
// Even if not all are used right now, we keep them for future use.
#define BLACK 0x0000
#define RED   0xF800
#define GREEN 0x07E0
#define WHITE 0xFFFF
#define GREY  0x8410

// =================== DATA STRUCTURE ===================
// We use a struct to group related pieces of information together.
// This makes it easier to pass around the "current date/time" as one object.
struct DateTimeInfo {
  String time;     // e.g. "14:37"
  String date;     // e.g. "2025-12-11"
  String weekday;  // e.g. "Thursday"
};

// A global variable that stores the latest received date/time info.
DateTimeInfo nowInfo;

// =================== GLOBAL STATE VARIABLES ===================

// This String buffer stores characters as they arrive from Serial until we see '\n'
String incoming = "";

// This flag tells us if we have successfully parsed at least one valid message.
bool hasValidTime = false;

// This flag tells the loop() function that the display needs to be redrawn.
bool needsRedraw = false;

// =================== SETUP FUNCTION ===================
// Runs once when the Arduino is powered on or reset.
void setup(void) {
  // Start the Serial connection at 9600 bits per second.
  // This must match the baud rate used by the Python script.
  Serial.begin(9600);

  // On some boards (e.g. Leonardo, Micro, Due), we need to wait
  // until the Serial connection is actually open.
  while (!Serial) {
    // Do nothing; just wait.
  }

  Serial.println("Arduino ready");

  // --------- INITIALISE THE TFT SCREEN ---------

  // Try to read the ID of the connected display.
  uint16_t ID = tft.readID();
  Serial.print("Found TFT ID = 0x");
  Serial.println(ID, HEX);

  // Some TFT shields return 0xD3D3 when they are "write-only".
  // In that case, we force the ID to a known working one (0x9481 here).
  if (ID == 0xD3D3) {
    ID = 0x9481;  // Force ID if write-only display
  }

  // Tell the TFT library which screen we are using.
  tft.begin(ID);

  // Set the rotation so that the screen is in landscape mode.
  // Try 0, 1, 2, 3 to see how it rotates.
  tft.setRotation(1);

  // Fill the whole screen with black to start with.
  tft.fillScreen(BLACK);
}

// =================== MAIN LOOP ===================
// This function runs again and again, forever.
void loop() {
  // Step 1: Check if any new serial data has arrived.
  readSerialMessage();

  // Step 2: If new valid data has arrived, redraw the display once.
  if (needsRedraw && hasValidTime) {
    updateDisplay();     // Redraw the TFT with the new time/date/weekday
    needsRedraw = false; // Reset the flag so we don't redraw unnecessarily
  }
}

// =================== SERIAL INPUT HANDLING ===================
// This function reads characters from the Serial buffer and builds them
// into a full line of text until it sees a newline '\n'.
void readSerialMessage() {
  // While there is at least one character waiting in the Serial buffer...
  while (Serial.available() > 0) {
    // Read a single character.
    char c = Serial.read();

    // If we reach a newline character '\n', we treat that as "end of message".
    if (c == '\n') {
      // Process the full message we have built up in 'incoming'.
      processIncoming(incoming);

      // Clear the buffer so we can start reading the next message.
      incoming = "";
    }
    // Ignore carriage returns '\r' (common on Windows).
    else if (c != '\r') {
      // Otherwise, add the character to our String buffer.
      incoming += c;
    }
  }
}

// =================== MESSAGE PARSING ===================
// This function takes a full line of text from the computer
// and splits it into time, date, and weekday.
//
// Expected format (exactly):
//    "HH:MM,YYYY-MM-DD,WeekdayName"
// Example:
//    "14:37,2025-12-11,Thursday"
void processIncoming(const String &msg) {
  // Find the positions of the two commas that separate the fields.
  int firstComma  = msg.indexOf(',');
  int secondComma = msg.indexOf(',', firstComma + 1);

  // If we can't find two commas, the message is in the wrong format.
  if (firstComma == -1 || secondComma == -1) {
    Serial.print("Bad message format: ");
    Serial.println(msg);
    return;  // Give up and wait for the next message.
  }

  // Extract the substrings:
  //  - From the start to the first comma: time
  //  - From after the first comma to the second comma: date
  //  - From after the second comma to the end: weekday
  nowInfo.time    = msg.substring(0, firstComma);
  nowInfo.date    = msg.substring(firstComma + 1, secondComma);
  nowInfo.weekday = msg.substring(secondComma + 1);

  // Mark that we now have valid time data.
  hasValidTime = true;

  // Tell the main loop that we should redraw the display.
  needsRedraw = true;

  // Print what we parsed to the Serial Monitor (for debugging).
  Serial.print("Parsed time: ");
  Serial.println(nowInfo.time);
  Serial.print("Parsed date: ");
  Serial.println(nowInfo.date);
  Serial.print("Parsed weekday: ");
  Serial.println(nowInfo.weekday);
}

// =================== TEXT DRAWING HELPER ===================
// This helper function draws a single line of text on the TFT.
//
// Parameters:
//   x, y   - the position of the text on the screen (in pixels)
//   sz     - the text size multiplier (1 is normal, 2 is double, etc.)
//   f      - which font to use (pointer to a GFXfont, e.g. &FreeSans12pt7b)
//   msg    - the actual text to draw
void showmsgXY(int x, int y, int sz, const GFXfont *f, const String msg) {
  // Choose the font to use (can be NULL for default built-in font)
  tft.setFont(f);

  // Set where the cursor (text starting point) will be.
  tft.setCursor(x, y);

  // Set the text colour. Here we use GREEN on a transparent background.
  tft.setTextColor(GREEN);

  // Set the size of the text.
  tft.setTextSize(sz);

  // Actually draw the text onto the screen.
  tft.print(msg);
}

// =================== SCREEN UPDATE ===================
// This function is called whenever new time/date/weekday data arrives.
// It clears the screen and redraws the three pieces of information
// at fixed positions using a nice font.
void updateDisplay() {
  // Clear the screen to black before drawing new content.
  tft.fillScreen(BLACK);

  // Also print to Serial so we can see what's happening in the Serial Monitor.
  Serial.println("Updating display with new DateTimeInfo...");
  Serial.print("Time: ");
  Serial.println(nowInfo.time);
  Serial.print("Date: ");
  Serial.println(nowInfo.date);
  Serial.print("Weekday: ");
  Serial.println(nowInfo.weekday);

  // Draw the weekday (e.g. "Thursday") near the top of the screen.
  // x = 20 pixels from the left
  // y = 40 pixels from the top
  // sz = 2 (text size)
  // font = FreeSans12pt7b (a clean sans-serif font)
  showmsgXY(20, 40, 2, &FreeSans12pt7b, nowInfo.weekday);

  // Draw the date (e.g. "2025-12-11") in the middle of the screen.
  showmsgXY(20, 100, 2, &FreeSans12pt7b, nowInfo.date);

  // Draw the time (e.g. "14:37") near the bottom of the screen in a larger size.
  showmsgXY(20, 180, 3, &FreeSans12pt7b, nowInfo.time);
}
