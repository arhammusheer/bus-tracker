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

// Retry mechanism
const int maxRetries = 5;            // Maximum number of retries
int retryCount = 0;                  // Current retry count
unsigned long lastRetryMillis = 0;   // Timestamp of the last retry
const unsigned long retryInterval = 5000;  // 5 seconds between retries

// We always wait a bit between updates of the display
unsigned long delaytime = 250;

void setup() {
  // Start the main serial communication (USB serial)
  Serial.begin(9600);

  // Start software serial communication
  mySerial.begin(9600);

  // Wake up the MAX72XX from power-saving mode
  lc.shutdown(0, false);

  // Set the brightness to normal initially
  lc.setIntensity(0, normalBrightness);

  // Clear the display
  lc.clearDisplay(0);

  lc.setChar(0, 0, "", true);
  lc.setChar(0, 1, "", true);
  lc.setChar(0, 2, "", true);
  lc.setChar(0, 3, "", true);

  // Print a startup message on the main serial
  Serial.println("Ready to receive variable-length numbers via Software Serial (pins 3 and 4)");
}

// This function converts seconds into MM.SS format and displays it on the 7-segment display
void displayTimeMMSS(int totalSeconds) {
  if (totalSeconds <= 0) {
    // Lower the brightness when no timing is available
    lc.setIntensity(0, lowBrightness);

    // Display ---- for negative numbers or zero
    for (int i = 0; i < 4; i++) {
      lc.setChar(0, i, '-', false);  // Display a dash on all 4 digits
    }

    // Retry sending 'R' if retries are available
    if (retryCount < maxRetries) {
      unsigned long currentMillis = millis();
      if (currentMillis - lastRetryMillis >= retryInterval) {
        mySerial.println('R');
        Serial.println("Sent 'R' to request new data due to zero or invalid time");
        lastRetryMillis = currentMillis;
        retryCount++;
      }
    } else {
      // If retries exhausted, show error message on the display
      displayError();
    }

    return;  // Exit the function early since we don't need to display time
  }

  // Reset retry count and restore brightness when valid time is available
  retryCount = 0;
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

// Function to display "Err" on the 7-segment display
void displayError() {
  lc.setIntensity(0, lowBrightness);  // Set to low brightness

  // Display "Err" across the 7-segment display
  lc.setChar(0, 3, 'E', false);
  lc.setChar(0, 2, 'r', false);
  lc.setChar(0, 1, 'r', false);
  lc.clearDisplay(0);  // Clear the 0th digit as we have no fourth letter
}

void loop() {
  unsigned long currentMillis = millis();  // Get the current time
  static String input = "";  // Use a static String to accumulate the input data

  // Echo any data from mySerial to main Serial
  while (mySerial.available()) {
    char received = mySerial.read();
    Serial.write(received);  // Echo the received character to the USB serial

    if (isDigit(received)) {  // Only add digits to the input string
      input += received;
    } else if (received == '\n' || received == '\r') {  // End of input (newline or carriage return)
      // If we received some digits, convert to seconds and display
      if (input.length() > 0 && input.length() <= 4) {  // Ensure length is valid (max 4 digits)
        totalSeconds = input.toInt();  // Convert the string to an integer

        // Check if the value is within a displayable range (maximum is 5999 seconds, or 99:59 MM.SS)
        if (totalSeconds <= 5999 && totalSeconds >= -1) {
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
          retryCount = 0;  // Reset the retry counter when valid data is received

          // Disable retries because we now have a valid countdown
          retryCount = maxRetries;  // This ensures retries are disabled
        } else {
          Serial.println("Error: Input too large for display.");
          displayError();
        }
      } else {
        Serial.println("Error: Invalid input.");
        displayError();
      }

      input = "";  // Clear the input string for the next message
    }
  }

  // Only handle retries if totalSeconds is zero or invalid (i.e., no valid data)
  if (totalSeconds <= 0 && retryCount < maxRetries) {
    if (currentMillis - lastRetryMillis >= retryInterval) {
      mySerial.println('R');  // Send 'R' to request new data
      Serial.println("Sent 'R' to request new data (no proper data available)");
      lastRetryMillis = currentMillis;
      retryCount++;
    }
  } else if (retryCount >= maxRetries) {
    // If retries exhausted, show error message on the display
    displayError();
  }

  // Update the countdown every second if totalSeconds is positive
  if (currentMillis - previousMillis >= interval && totalSeconds > 0) {
    previousMillis = currentMillis;  // Save the last time we updated the display

    totalSeconds--;  // Decrease totalSeconds by 1

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
