# Required Arduino libraries

Install these from the Arduino IDE Library Manager (Sketch -> Include
Library -> Manage Libraries):

| Library            | Author              | Used by              |
|--------------------|---------------------|----------------------|
| DHT sensor library | Adafruit            | smart_pot_mega.ino   |
| Adafruit Unified Sensor | Adafruit       | dependency of DHT    |
| ESP8266WiFi        | (built-in, ESP core)| esp8266_wifi.ino     |
| ESP8266HTTPClient  | (built-in, ESP core)| esp8266_wifi.ino     |

## Board support packages

- **Arduino Mega 2560**: comes built-in with the Arduino IDE.
- **ESP8266**: add this URL in *File -> Preferences -> Additional Board
  Manager URLs*:
  ```
  http://arduino.esp8266.com/stable/package_esp8266com_index.json
  ```
  then install "esp8266 by ESP8266 Community" from Boards Manager.

## Upload notes

- Upload `smart_pot_mega.ino` to the Mega first (Tools -> Board -> Arduino
  Mega or Mega 2560).
- Disconnect the ESP8266 from the Mega's TX1/RX1 while uploading via USB.
- Upload `esp8266_wifi.ino` to the ESP separately. ESP-01 modules need a
  USB-to-serial adapter and GPIO0 pulled to GND during flashing.
