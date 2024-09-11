#include "LedControl.h"
#include <SoftwareSerial.h>

// Pin 12 is connected to DataIn, pin 11 to CLK, and pin 10 to LOAD. We have a single MAX72XX.
LedControl lc = LedControl(12, 11, 10, 1);

// Create a software serial object on pins 3 (RX) and 4 (TX)
SoftwareSerial mySerial(3, 4);  // RX on pin 3, TX on pin 4

// Set brightness levels
const int lowBrightness = 1;     // Low brightness level when no timings are available
const int normalBrightness = 9;  // Normal brightness level when timings are available

// Countdown variables
int totalSeconds = 0;           // Total countdown time in seconds
unsigned long previousMillis = 0;  // Store the last time we updated the countdown
const unsigned long interval = 1000;  // 1 second interval for countdown

// We always wait a bit between updates of the display
unsigned long delaytime = 250;

void setup() {
  // Start the main serial communication (USB serial)
  Serial.begin(115200);

  // Start software serial communication
  mySerial.begin(115200);

  // Wake up the MAX72XX from power-saving mode
  lc.shutdown(0, false);

  // Set the brightness to normal initially
  lc.setIntensity(0, normalBrightness);

  // Clear the display
  lc.clearDisplay(0);

  // Print a startup message on the main serial
  Serial.println("Ready to receive 4-digit numbers via Software Serial (pins 3 and 4)");
}

// This function converts seconds into MM.SS format and displays it on the 7-segment display
void displayTimeMMSS(int totalSeconds) {
  if (totalSeconds <= 0) {
    // Lower the brightness when no timing is available
    lc.setIntensity(0, lowBrightness);

    // Display ---- for negative numbers
    for (int i = 0; i < 4; i++) {
      lc.setChar(0, i, '-', false);  // Display a dash on all 4 digits
    }
    return;  // Exit the function early since we don't need to display time
  }

  // Restore brightness when valid time is available
  lc.setIntensity(0, normalBrightness);

  int minutes = totalSeconds / 60;  // Calculate minutes
  int seconds = totalSeconds % 60;  // Calculate remaining seconds

  // Check if minutes are greater than 9 to determine whether to show the leading zero
  if (minutes >= 10) {
    lc.setDigit(0, 3, minutes / 10, false);   // Tens place of minutes
  } else {
    lc.clearDisplay(0);  // Clear the digit for the leading zero
  }

  lc.setDigit(0, 2, minutes % 10, true);    // Units place of minutes, enable decimal point

  // Display the seconds on the last two digits
  lc.setDigit(0, 1, seconds / 10, false);   // Tens place of seconds
  lc.setDigit(0, 0, seconds % 10, false);   // Units place of seconds
}

void loop() {
  unsigned long currentMillis = millis();  // Get the current time

  // Check if data is available on the software serial
  if (mySerial.available() >= 4) {  // Ensure that we have at least 4 bytes to read
    char input[5];  // To store the 4 digits and the null-terminator
    for (int i = 0; i < 4; i++) {
      input[i] = mySerial.read();  // Read each byte
    }
    input[4] = '\0';  // Null-terminate the string

    // Convert the input string to an integer representing total seconds
    totalSeconds = atoi(input);

    // Display the time in MM.SS format on the 7-segment display
    displayTimeMMSS(totalSeconds);

    // Echo the received seconds and converted time on the main serial (USB)
    Serial.print("Received seconds: ");
    Serial.println(totalSeconds);
    
    int minutes = totalSeconds / 60;
    int seconds = totalSeconds % 60;
    Serial.print("Time in MM.SS: ");
    Serial.print(minutes);
    Serial.print(".");
    Serial.println(seconds);

    // Reset the previousMillis to start countdown from the current time
    previousMillis = currentMillis;
  }

  // Check if enough time (1 second) has passed to update the countdown
  if (currentMillis - previousMillis >= interval && totalSeconds > 0) {
    // Save the last time we updated the display
    previousMillis = currentMillis;

    // Decrease totalSeconds by 1
    totalSeconds--;

    // Display the updated time in MM.SS format on the 7-segment display
    displayTimeMMSS(totalSeconds);

     // Print the countdown time on the main serial (USB)
    int minutes = totalSeconds / 60;
    int seconds = totalSeconds % 60;
    Serial.print("Counting down: ");
    Serial.print(minutes);
    Serial.print(".");
    Serial.println(seconds);
  }
}