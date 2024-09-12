#include <WiFi.h>
#include <HTTPClient.h>
#include <EEPROM.h>
#include "HardwareSerial.h"

// Define the pin that will trigger the credential input mode
#define MODE_PIN 14  // Change to your pin number

// EEPROM addresses to store SSID, password, and HTTP URL
#define EEPROM_SIZE 256  // Adjust size as needed
#define SSID_ADDR 0
#define PASSWORD_ADDR 32
#define URL_ADDR 96  // HTTP URL starts here

String ssid = "";
String password = "";
String httpUrl = "";
String lastHttpResponse = "";  // New variable to store the last HTTP response

unsigned long delayInterval = 0;
unsigned long lastMillis = 0;

void setup() {
  // Initialize Serial for debugging
  Serial.begin(9600);

  // Initialize Serial1 for RX2/TX2 output (on pins 16 and 17 for ESP32)
  Serial1.begin(9600, SERIAL_8N1, 16, 17);

  // Initialize EEPROM
  EEPROM.begin(EEPROM_SIZE);

  // Set the pin as input (add a pull-down resistor if needed)
  pinMode(MODE_PIN, INPUT);

  // Check if the pin is activated (pin is connected to HIGH)
  if (digitalRead(MODE_PIN) == HIGH) {
    // Enter serial input mode to get SSID, password, and HTTP URL
    getCredentialsOverSerial();
  } else {
    // Normal Wi-Fi mode, load credentials and URL from EEPROM
    loadCredentialsFromEEPROM();
    if (!areCredentialsValid()) {
      // If any credentials or URL are missing or invalid, request them over serial
      Serial.println("Missing or invalid credentials. Please input over serial.");
      getCredentialsOverSerial();
    } else {
      // Try to connect to Wi-Fi
      connectToWiFi();
      makeHttpCall();  // Make initial HTTP call
    }
  }
}
void loop() {
  // Check if it's time for the next HTTP call
  unsigned long currentMillis = millis();
  if (currentMillis - lastMillis >= delayInterval * 1000) {
    lastMillis = currentMillis;
    makeHttpCall();
  }

   // Check if Serial1 received any data
  if (Serial1.available()) {
    char received = Serial1.read();
    if (received == 'R') {
      delay(5); // Delay for Serial to settle
      // Send the last HTTP response over Serial1
      Serial1.println(lastHttpResponse);
      Serial.print("Sent last HTTP response over Serial1. Response: ");
      Serial.println(lastHttpResponse);
    }
  }


  // Check if Serial (USB/Monitor) received any data
  if (Serial.available()) {
    String receivedSerialCommand = Serial.readStringUntil('\n');
    receivedSerialCommand.trim();  // Remove any newline characters or extra spaces

    if (receivedSerialCommand.equals("SEND")) {
      // Send the last HTTP response over Serial1 when "SEND" command is received
      Serial1.println(lastHttpResponse);
      Serial.print("Sent last HTTP response over Serial1 in response to SEND command. Response: ");
      Serial.println(lastHttpResponse);
    }
  }
}

// Function to get SSID, Password, and HTTP URL from Serial input
void getCredentialsOverSerial() {
  ssid = "";
  password = "";
  httpUrl = "";

  Serial.println("Enter SSID: ");
  while (ssid.isEmpty()) {
    if (Serial.available()) {
      ssid = Serial.readStringUntil('\n');
      ssid.trim();  // Remove any trailing newline or spaces
      if (ssid.isEmpty()) {
        Serial.println("SSID cannot be empty. Please enter SSID: ");
      }
    }
  }

  Serial.println("Enter Password: ");
  while (password.isEmpty()) {
    if (Serial.available()) {
      password = Serial.readStringUntil('\n');
      password.trim();  // Remove any trailing newline or spaces
      if (password.isEmpty()) {
        Serial.println("Password cannot be empty. Please enter Password: ");
      }
    }
  }

  Serial.println("Enter HTTP URL: ");
  while (httpUrl.isEmpty()) {
    if (Serial.available()) {
      httpUrl = Serial.readStringUntil('\n');
      httpUrl.trim();  // Remove any trailing newline or spaces
      if (httpUrl.isEmpty()) {
        Serial.println("URL cannot be empty. Please enter a valid HTTP URL: ");
      }
    }
  }

  // Store the credentials and URL in EEPROM
  storeCredentialsInEEPROM();
  Serial.println("Credentials and URL saved to EEPROM!");

  // Connect to Wi-Fi after storing credentials and URL
  connectToWiFi();
  makeHttpCall();
}

// Function to store SSID, password, and HTTP URL to EEPROM
void storeCredentialsInEEPROM() {
  // Write SSID to EEPROM
  for (int i = 0; i < ssid.length(); i++) {
    EEPROM.write(SSID_ADDR + i, ssid[i]);
  }
  EEPROM.write(SSID_ADDR + ssid.length(), '\0');  // Null-terminate the SSID

  // Write password to EEPROM
  for (int i = 0; i < password.length(); i++) {
    EEPROM.write(PASSWORD_ADDR + i, password[i]);
  }
  EEPROM.write(PASSWORD_ADDR + password.length(), '\0');  // Null-terminate the password

  // Write HTTP URL to EEPROM
  for (int i = 0; i < httpUrl.length(); i++) {
    EEPROM.write(URL_ADDR + i, httpUrl[i]);
  }
  EEPROM.write(URL_ADDR + httpUrl.length(), '\0');  // Null-terminate the URL

  // Commit changes to EEPROM
  EEPROM.commit();
}

// Function to load SSID, password, and HTTP URL from EEPROM
void loadCredentialsFromEEPROM() {
  char ssidBuffer[32];
  char passwordBuffer[64];
  char urlBuffer[160];

  // Read SSID from EEPROM
  for (int i = 0; i < 32; i++) {
    ssidBuffer[i] = EEPROM.read(SSID_ADDR + i);
    if (ssidBuffer[i] == '\0') break;  // Stop reading if we hit a null terminator
  }
  ssid = String(ssidBuffer);

  // Read password from EEPROM
  for (int i = 0; i < 64; i++) {
    passwordBuffer[i] = EEPROM.read(PASSWORD_ADDR + i);
    if (passwordBuffer[i] == '\0') break;  // Stop reading if we hit a null terminator
  }
  password = String(passwordBuffer);

  // Read HTTP URL from EEPROM
  for (int i = 0; i < 160; i++) {
    urlBuffer[i] = EEPROM.read(URL_ADDR + i);
    if (urlBuffer[i] == '\0') break;  // Stop reading if we hit a null terminator
  }
  httpUrl = String(urlBuffer);

  // Debug print the loaded credentials (Don't print passwords in production code)
  Serial.println("Loaded SSID from EEPROM: " + ssid);
  Serial.println("Loaded HTTP URL from EEPROM: " + httpUrl);
}

// Function to check if the loaded credentials are valid
bool areCredentialsValid() {
  return !ssid.isEmpty() && !password.isEmpty() && !httpUrl.isEmpty();
}

// Function to connect to Wi-Fi
void connectToWiFi() {
  Serial.println("Connecting to Wi-Fi...");
  WiFi.begin(ssid.c_str(), password.c_str());

  int retryCount = 0;
  while (WiFi.status() != WL_CONNECTED && retryCount < 20) {
    delay(500);
    Serial.print(".");
    retryCount++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConnected to Wi-Fi!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nFailed to connect to Wi-Fi");
  }
}

// Function to make an HTTP call
void makeHttpCall() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    // Specify the URL from EEPROM
    http.begin(httpUrl);

    // Send the request and get the response
    int httpCode = http.GET();

    // Check the returning HTTP code
    if (httpCode > 0) {
      // Get the response payload, which should be a number in seconds
      String payload = http.getString();
      lastHttpResponse = payload;  // Store the last response in global variable

      int responseSeconds = payload.toInt();  // Convert response to integer

      // Output only the seconds number on Serial1 (TX2/RX2)
      Serial1.println(payload);  // Output only seconds on TX2
      Serial.print("Seconds: ");
      Serial.println(payload);

      // Calculate the delay for the next call based on the response
      calculateDelay(responseSeconds);
    } else {
      Serial.println("Error in HTTP request");
    }

    // Close connection
    http.end();
  } else {
    Serial.println("Wi-Fi not connected!");
  }
}

// Function to calculate smart delay based on server response
void calculateDelay(int secondsFromNow) {
  // More than 1 hour (3600 seconds)
  if (secondsFromNow > 3600) {
    delayInterval = 300;  // Check every 5 minutes
  }
  // 1 hour left, check every 1 minute
  else if (secondsFromNow <= 3600 && secondsFromNow > 600) {
    delayInterval = 60;  // Check every 1 minute
  }
  // 10 minutes left, check every 30 seconds
  else if (secondsFromNow <= 600 && secondsFromNow > 120) {
    delayInterval = 30;  // Check every 30 seconds
  }
  // 2 minutes left, check every 5 seconds
  else if (secondsFromNow <= 120 && secondsFromNow >= 0) {
    delayInterval = 5;  // Check every 5 seconds
  } else {
    delayInterval = 60;  // If no bus time, delay should be a minute
  }

  Serial.print("Next check in ");
  Serial.print(delayInterval);
  Serial.println(" seconds.");
}
