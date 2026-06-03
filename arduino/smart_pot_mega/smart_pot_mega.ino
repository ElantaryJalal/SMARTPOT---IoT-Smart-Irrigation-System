/*
 * SMARTPOT - Smart Irrigation System
 * Arduino Mega 2560 firmware
 *
 * Reads soil moisture, air temperature/humidity, light, rain and water-tank
 * level, computes daily plant water needs using the Turc evapotranspiration
 * formula, drives a water pump through a relay, and forwards readings to an
 * ESP8266 (WiFi) over Serial1 so they can be posted to the web dashboard.
 *
 * Hardware wiring (per design report):
 *   DHT22 (air T/RH)      -> D7
 *   Soil moisture sensor  -> A2  (analog)
 *   Photoresistor (light) -> A1  (analog, 10k pull-down)
 *   Rain sensor           -> A0  (analog)
 *   Ultrasonic HC-SR04    -> TRIG D9, ECHO D10
 *   Relay (pump)          -> D5
 *   ESP8266 (Serial1)     -> TX1=D18, RX1=D19
 *
 * Project: PFE Licence SMI - FSBM, Universite Hassan II Casablanca (2021/2022)
 */

#include <DHT.h>

// ---------- Pins ----------
#define DHT_PIN          7
#define DHT_TYPE         DHT22
#define SOIL_PIN        A2
#define LIGHT_PIN       A1
#define RAIN_PIN        A0
#define US_TRIG_PIN      9
#define US_ECHO_PIN     10
#define RELAY_PIN        5

// ---------- Crop / soil constants (tune for your plant) ----------
// Crop coefficient Kc (mid-season value; see report Figure 24)
const float KC = 1.0;
// Field capacity (theta_CC) in % volumetric water content
const float THETA_CC = 35.0;
// Permanent wilting point (theta_PF) in %
const float THETA_PF = 12.0;
// Root depth Z in mm
const float ROOT_DEPTH_MM = 150.0;
// Pump flow rate (mL per second) - measure yours with a graduated cup
const float PUMP_FLOW_ML_S = 30.0;
// Pot surface area in cm^2 (1 mm of water on this area = X mL)
const float POT_AREA_CM2 = 200.0;
// Water tank empty/full distances measured by ultrasonic, in cm
const float TANK_EMPTY_CM = 18.0;  // distance when tank is empty
const float TANK_FULL_CM  =  3.0;  // distance when tank is full
const float TANK_VOL_L    =  1.0;  // total tank volume

// ---------- Timing ----------
const unsigned long SAMPLE_INTERVAL_MS = 30UL * 60UL * 1000UL; // 30 min
const unsigned long SEND_INTERVAL_MS   = 60UL * 1000UL;        // 1 min uplink
const int LIGHT_NIGHT_THRESHOLD   = 10;   // below = night, pause loop
const int LIGHT_SUNRISE_THRESHOLD = 700;  // above = sunrise, allowed to water

// ---------- Globals ----------
DHT dht(DHT_PIN, DHT_TYPE);

// Daily accumulators (reset every morning)
float sumT = 0, sumHa = 0, sumRG = 0, sumPu = 0;
int   nbrMesure = 0;
float ds_start = 0;          // soil moisture at start of day
bool  dayStarted = false;
bool  wateredToday = false;

unsigned long lastSampleMs = 0;
unsigned long lastSendMs   = 0;

// ---------- Helpers ----------

// Map raw soil ADC to % volumetric water content.
// Calibrate: dry-air = ~1023, fully wet = ~300 on most capacitive probes.
float readSoilMoisturePct() {
  int raw = analogRead(SOIL_PIN);
  float pct = map(raw, 1023, 300, 0, 100);
  if (pct < 0) pct = 0;
  if (pct > 100) pct = 100;
  return pct;
}

// Read photoresistor as a relative light level (0..1023).
int readLight() {
  return analogRead(LIGHT_PIN);
}

// Convert the analog light reading to global radiation RG in Lux,
// then to W/m^2 equivalent via the 0.0079 factor used in the report.
float readRG_W_m2() {
  // Photoresistor reading is not lux-calibrated; treat the ADC value as a
  // proxy and apply the same 0.0079 factor referenced in the report so the
  // Turc formula gets a usable RG estimate.
  return analogRead(LIGHT_PIN) * 0.0079;
}

// Rain sensor: returns daily contribution in mm (very rough).
// The analog sensor outputs ~1023 dry, ~300 wet. We translate a "wet" event
// into a small effective precipitation increment.
float readRainMm() {
  int raw = analogRead(RAIN_PIN);
  if (raw > 800) return 0.0;          // dry
  if (raw > 500) return 0.2;          // light rain
  return 0.6;                          // heavy rain
}

// Ultrasonic distance in cm
float readDistanceCm() {
  digitalWrite(US_TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(US_TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(US_TRIG_PIN, LOW);
  long dur = pulseIn(US_ECHO_PIN, HIGH, 30000UL); // 30 ms timeout
  if (dur == 0) return -1;
  return (dur * 0.0343f) / 2.0f;
}

// Water remaining in the tank, in litres
float tankLitres() {
  float d = readDistanceCm();
  if (d < 0) return -1;
  float pct = (TANK_EMPTY_CM - d) / (TANK_EMPTY_CM - TANK_FULL_CM);
  if (pct < 0) pct = 0;
  if (pct > 1) pct = 1;
  return pct * TANK_VOL_L;
}

// Turc reference evapotranspiration ET0 (mm/day).
// Ha in %, T in degC, RG in W/m^2.
float computeET0(float T, float Ha, float RG) {
  float base = 0.13f * (RG + 50.0f) * (T / (T + 15.0f));
  if (Ha <= 50.0f) {
    base *= (1.0f + (50.0f - Ha) / 70.0f);
  }
  return base;
}

// Maximum irrigation dose D_MAX (mm), keeps soil <= field capacity
float computeDmax(float thetaI) {
  float dose = ((THETA_CC - thetaI) / 100.0f) * ROOT_DEPTH_MM;
  if (dose < 0) dose = 0;
  return dose;
}

// Convert a millimetre water depth to millilitres for this pot
float mmToMl(float mm) {
  // 1 mm over area cm^2 -> area * 0.1 mL  (since 1 mm = 0.1 cm)
  return mm * POT_AREA_CM2 * 0.1f;
}

// Activate the pump for the given volume in mL
void runPumpForMl(float ml) {
  if (ml <= 0) return;
  unsigned long durationMs = (unsigned long)((ml / PUMP_FLOW_ML_S) * 1000.0f);
  if (durationMs > 60000UL) durationMs = 60000UL; // hard safety cap: 60 s
  digitalWrite(RELAY_PIN, LOW);  // most relay modules are active-LOW
  delay(durationMs);
  digitalWrite(RELAY_PIN, HIGH);
}

// Send a JSON-like CSV frame to the ESP8266 over Serial1.
// The ESP will wrap it in an HTTP POST.
void sendToEsp(float T, float Ha, float soilPct, float light,
               float tankL, bool pumpOn) {
  Serial1.print("DATA,");
  Serial1.print(T, 1);     Serial1.print(',');
  Serial1.print(Ha, 1);    Serial1.print(',');
  Serial1.print(soilPct, 1); Serial1.print(',');
  Serial1.print(light, 0); Serial1.print(',');
  Serial1.print(tankL, 2); Serial1.print(',');
  Serial1.println(pumpOn ? 1 : 0);
}

// ---------- Setup ----------
void setup() {
  Serial.begin(9600);     // USB debug
  Serial1.begin(9600);    // link to ESP8266
  dht.begin();

  pinMode(US_TRIG_PIN, OUTPUT);
  pinMode(US_ECHO_PIN, INPUT);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH);  // pump OFF (active-LOW relay)

  Serial.println(F("SMARTPOT booting..."));
}

// ---------- Main loop ----------
void loop() {
  unsigned long now = millis();
  int light = readLight();

  // --- Sample every SAMPLE_INTERVAL_MS during daylight ---
  if (light >= LIGHT_NIGHT_THRESHOLD &&
      (now - lastSampleMs >= SAMPLE_INTERVAL_MS || lastSampleMs == 0)) {

    lastSampleMs = now;

    float T  = dht.readTemperature();
    float Ha = dht.readHumidity();
    float soil = readSoilMoisturePct();
    float RG = readRG_W_m2();
    float Pu = readRainMm();

    if (!dayStarted) {
      ds_start = soil;
      dayStarted = true;
      wateredToday = false;
      sumT = sumHa = sumRG = sumPu = 0;
      nbrMesure = 0;
    }

    if (!isnan(T) && !isnan(Ha)) {
      sumT  += T;
      sumHa += Ha;
      sumRG += RG;
      sumPu += Pu;
      nbrMesure++;
    }

    Serial.print(F("[sample] T="));   Serial.print(T);
    Serial.print(F(" Ha="));          Serial.print(Ha);
    Serial.print(F(" soil%="));       Serial.print(soil);
    Serial.print(F(" light="));       Serial.print(light);
    Serial.print(F(" rainMm="));      Serial.println(Pu);
  }

  // --- At sunrise: decide irrigation and reset day ---
  if (dayStarted && light >= LIGHT_SUNRISE_THRESHOLD && !wateredToday
      && nbrMesure > 0) {

    float meanT  = sumT  / nbrMesure;
    float meanHa = sumHa / nbrMesure;
    float meanRG = sumRG / nbrMesure;
    float Pu     = sumPu;                       // cumulative for the day
    float soilNow = readSoilMoisturePct();
    float DS = soilNow - ds_start;              // mm-equivalent if you scale,
                                                // here kept in % for the doc

    float ET0 = computeET0(meanT, meanHa, meanRG);
    float ETc = KC * ET0;
    // Beau = ETc - Pu - DS  (DS expressed in mm of water equivalent)
    float DS_mm = (DS / 100.0f) * ROOT_DEPTH_MM;
    float Beau = ETc - Pu - DS_mm;

    float Dmax = computeDmax(soilNow);

    Serial.print(F("[decision] ET0="));  Serial.print(ET0);
    Serial.print(F(" ETc="));            Serial.print(ETc);
    Serial.print(F(" Beau="));           Serial.print(Beau);
    Serial.print(F(" Dmax="));           Serial.println(Dmax);

    float dose = Beau;
    if (dose > Dmax) dose = Dmax;
    if (dose < 0)    dose = 0;

    if (dose > 0.1f) {
      float ml = mmToMl(dose);
      Serial.print(F("[pump] dispensing mL="));
      Serial.println(ml);
      runPumpForMl(ml);
    }
    wateredToday = true;
  }

  // --- Night: reset the day flag so a fresh cycle starts tomorrow ---
  if (light < LIGHT_NIGHT_THRESHOLD && dayStarted && wateredToday) {
    dayStarted = false;
  }

  // --- Periodic uplink to the ESP8266 / web dashboard ---
  if (now - lastSendMs >= SEND_INTERVAL_MS) {
    lastSendMs = now;
    float T   = dht.readTemperature();
    float Ha  = dht.readHumidity();
    float soil = readSoilMoisturePct();
    float tank = tankLitres();
    bool  pumpOn = (digitalRead(RELAY_PIN) == LOW);

    if (!isnan(T) && !isnan(Ha)) {
      sendToEsp(T, Ha, soil, light, tank, pumpOn);
    }
  }

  delay(200);
}
