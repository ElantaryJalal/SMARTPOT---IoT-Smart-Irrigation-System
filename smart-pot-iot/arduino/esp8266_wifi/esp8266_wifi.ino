/*
 * SMARTPOT - ESP8266 WiFi bridge
 *
 * Receives "DATA,T,Ha,soil,light,tankL,pump" lines from the Arduino Mega
 * over the serial link and POSTs them as JSON to the PHP backend.
 *
 * Board: Generic ESP8266 Module (ESP-01) or NodeMCU
 * IDE   : Arduino IDE with esp8266 boards package
 */

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>

// ----- CONFIGURE THESE -----
const char* WIFI_SSID     = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
// URL of the PHP receiver. Update host/port to wherever you deploy.
const char* API_URL       = "http://192.168.1.10/smartpot/api/receive_data.php";
// Pot identifier - matches the "code" generated at user registration
const char* POT_CODE      = "DEMO-CODE-001";
// ---------------------------

void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 20000UL) {
    delay(500);
    Serial.print('.');
  }
  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("WiFi OK, IP=");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("WiFi FAILED, will retry on next frame");
  }
}

bool postJson(const String& payload) {
  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
    if (WiFi.status() != WL_CONNECTED) return false;
  }
  WiFiClient client;
  HTTPClient http;
  if (!http.begin(client, API_URL)) return false;
  http.addHeader("Content-Type", "application/json");
  int code = http.POST(payload);
  String resp = http.getString();
  http.end();
  Serial.print("POST ");
  Serial.print(code);
  Serial.print(": ");
  Serial.println(resp);
  return code >= 200 && code < 300;
}

// Parse "DATA,T,Ha,soil,light,tankL,pump" into a JSON payload
String frameToJson(const String& line) {
  // Skip the leading "DATA,"
  int start = line.indexOf(',');
  if (start < 0) return "";
  String s = line.substring(start + 1);

  float vals[6] = {0,0,0,0,0,0};
  int idx = 0;
  while (s.length() > 0 && idx < 6) {
    int comma = s.indexOf(',');
    String token = (comma < 0) ? s : s.substring(0, comma);
    vals[idx++] = token.toFloat();
    if (comma < 0) break;
    s = s.substring(comma + 1);
  }

  String j  = "{\"code\":\"";
  j += POT_CODE;
  j += "\",\"temperature\":"; j += String(vals[0], 1);
  j += ",\"humidity\":";      j += String(vals[1], 1);
  j += ",\"soil\":";          j += String(vals[2], 1);
  j += ",\"light\":";         j += String(vals[3], 0);
  j += ",\"tank\":";          j += String(vals[4], 2);
  j += ",\"pump\":";          j += String((int)vals[5]);
  j += "}";
  return j;
}

void setup() {
  Serial.begin(9600);
  delay(200);
  connectWiFi();
}

String buffer;

void loop() {
  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c == '\n') {
      buffer.trim();
      if (buffer.startsWith("DATA,")) {
        String payload = frameToJson(buffer);
        if (payload.length() > 0) postJson(payload);
      }
      buffer = "";
    } else if (c != '\r') {
      buffer += c;
      if (buffer.length() > 200) buffer = "";  // overflow guard
    }
  }
  delay(10);
}
