# Bus Tracker Project

Welcome to the **Bus Tracker** project! This project involves an ESP32 and Arduino setup that interacts with a server via HTTP to retrieve timing data for bus schedules and display it on a MAX72XX 7-segment LED display. It communicates wirelessly, receiving data from a server and then counting down the time on the display. Here's a quick guide to what the code does and how to set everything up.

## Demo Image
<img width="867" alt="image" src="https://github.com/user-attachments/assets/50544e3f-4da6-460f-877f-24a567a0607d">


## Hardware Requirements
- **ESP32**: To handle Wi-Fi connectivity and HTTP requests.
- **MAX7221 7-Segment LED Display**: Displays the bus countdown times in MM:SS format. (I've used MAX7221CNG+)
- **UART1/SoftwareSerial**: Two UART interfaces: one for debugging, and one for receiving timing data.
- **EEPROM**: To store Wi-Fi credentials and server URL.
- **Mode Button**: A button to switch between normal Wi-Fi mode and a configuration mode to input new Wi-Fi credentials and HTTP server URL.

## Software Components
For API request and bus data sources refer to the home-apis repository. https://github.com/arhammusheer/croissant-home-apis

### ESP32 Part
1. **Wi-Fi Connectivity**: 
   - The ESP32 connects to a Wi-Fi network using credentials stored in EEPROM. If no valid credentials are stored or the mode button is pressed, it will prompt for new credentials via Serial.
   
2. **HTTP Requests**:
   - The ESP32 fetches timing data (in seconds) from an HTTP server. The response is used to display the countdown on a 7-segment display. If the bus arrival time is more than an hour away, it reduces the frequency of updates to avoid excessive HTTP requests.

3. **EEPROM Handling**:
   - The Wi-Fi SSID, password, and HTTP URL are stored in the EEPROM for persistence, even after power is cycled.

4. **UART Communication**:
   - `Serial1` (pins 16 and 17) sends the timing data (in seconds) to the Arduino for display purposes.

### Arduino Part (Countdown Display)
1. **SoftwareSerial Communication**:
   - The Arduino listens on `SoftwareSerial` for incoming timing data from the ESP32 (4-digit numbers representing seconds).

2. **MAX72XX 7-Segment Display**:
   - The display shows the bus arrival countdown in MM:SS format. When no valid timing data is available, it shows `----` and dims the brightness. Otherwise, it shows the countdown in real time with normal brightness.

3. **Countdown Logic**:
   - The Arduino continuously updates the countdown once it receives new timing data and shows it on the display.

## How to Use

### Setup Wi-Fi Credentials
1. **First-Time Setup**: 
   - If no Wi-Fi credentials or HTTP URL are stored in EEPROM, or if the **mode button** (on pin 14) is pressed during startup, the ESP32 will enter a special serial input mode where you can enter your Wi-Fi SSID, password, and HTTP URL.
   
2. **Normal Operation**:
   - On startup, the ESP32 attempts to load credentials from EEPROM and connect to Wi-Fi. If successful, it will start making HTTP requests to retrieve the bus timing data.
   
3. **Modifying Credentials**:
   - If you need to update the Wi-Fi credentials or HTTP URL, hold the mode button during reset and follow the serial prompts to input new credentials.

### Running the Bus Tracker
1. **Power up the system**.
2. If Wi-Fi credentials are valid, the ESP32 will connect to the network and make an HTTP request to the provided URL to retrieve bus timing data (in seconds).
3. The timing data is sent over UART to the Arduino, which displays it on the 7-segment LED in MM:SS format.
4. The countdown will automatically update every second. Once the bus is expected within 2 minutes, the display will update more frequently (every 5 seconds).

### Example Use Case
1. Input your Wi-Fi credentials and server URL through Serial when prompted.
2. The ESP32 connects to the Wi-Fi and makes an HTTP request to get bus arrival time.
3. The Arduino displays the bus arrival countdown, updating every second as the bus approaches.
4. Once the bus is close, the LED display updates more frequently to ensure timely notifications.

## How it Works (Under the Hood)

- **Smart Delay**: Based on how far away the bus is, the system adjusts the frequency of HTTP requests. For example, if the bus is over an hour away, the requests are made less frequently, whereas when the bus is within a few minutes, it will check more often.
  
- **EEPROM Persistence**: Credentials and the HTTP URL are saved to EEPROM, so even after a restart, the system retains the Wi-Fi settings and server URL, avoiding the need for reconfiguration.

- **Countdown Logic**: The bus arrival time is received in seconds, converted to MM:SS, and displayed on the MAX72XX. If no valid data is available, it dims the display and shows dashes (`----`).

## Schematics
Download as PDF [Bus tracker schematics.pdf](https://github.com/user-attachments/files/16958053/Bus.tracker.schematics.pdf)

![Bus tracker schematics](https://github.com/user-attachments/assets/4a518af6-8a03-4688-a62d-755465d72eec)

## Request Flow
<img width="830" alt="image" src="https://github.com/user-attachments/assets/6ce0a8ba-cf36-4b69-af0f-572f1980ae57">


## License
[MIT](https://choosealicense.com/licenses/mit/)