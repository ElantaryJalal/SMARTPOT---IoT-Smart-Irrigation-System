# SMARTPOT - IoT Smart Irrigation System

> Final-year undergraduate project (Licence Fondamentale, Sciences Mathematiques
> & Informatique). FSBM, Universite Hassan II de Casablanca - 2021/2022.

An automated smart irrigation system for indoor plants. A pot fitted with
soil, climate and water-level sensors decides on its own when and how much
to water a plant, while pushing live data to a web dashboard over WiFi.

The watering decision is not a naive "if dry, water" rule - it is based on
the **Turc reference evapotranspiration formula** (ET0) combined with crop
coefficients (Kc), root depth and rainfall, so the pot only delivers the
water the plant actually needs that day.

| Layer       | Stack                                                       |
|-------------|-------------------------------------------------------------|
| Firmware    | C++ on **Arduino Mega 2560**, **ESP8266** for WiFi          |
| Sensors     | DHT22, capacitive soil moisture, photoresistor, HC-SR04, rain sensor |
| Actuator    | 5 V relay + DC water pump                                   |
| Backend     | **PHP 8 + MySQL 8** (Apache)                                |
| Frontend    | Server-rendered HTML/CSS, auto-refresh dashboard            |

---

## Features

- **Autonomous watering** - the pot computes its own daily water budget and
  refuses to over- or under-water.
- **Live dashboard** - temperature, humidity and tank level update every
  30 s with circular gauges.
- **History** - the last 200 readings are kept, sortable by time.
- **Tank-low alert** - the ultrasonic sensor measures the reservoir; when
  it drops below 0.2 L the dashboard shows a refill banner.
- **Multi-user** - each device is paired with a user via a unique pot code
  printed on the device, so several owners can use the same backend.
- **Cheap & repairable** - all components are under 30 USD total and
  available from generic electronics shops.

---

## How the watering algorithm works

Per day, the firmware accumulates samples every 30 minutes during daylight
and at sunrise computes:

1. **Turc reference evapotranspiration** ET0 (mm/day)

   - if humidity Ha > 50 %:
     `ET0 = 0.13 * (RG + 50) * T / (T + 15)`
   - if humidity Ha <= 50 %:
     `ET0 = 0.13 * (RG + 50) * T / (T + 15) * (1 + (50 - Ha) / 70)`

   where T is mean air temperature (degC), Ha mean air humidity (%) and
   RG global radiation (W/m^2, approximated from the photoresistor).

2. **Crop evapotranspiration** `ETc = Kc * ET0` with Kc per growth stage
   (see report, Figure 24).
3. **Net water need** `Beau = ETc - Pu - DS`, where Pu is rainfall and DS
   the change in soil moisture over the day.
4. **Maximum dose** `D_MAX = ((theta_CC - theta_i) / 100) * Z` to avoid
   exceeding the soil's field capacity.
5. The pump is opened at sunrise for the time corresponding to
   `min(Beau, D_MAX)` converted to millilitres through the pump's measured
   flow rate.

The Arduino sketch implements every step of this in
[`arduino/smart_pot_mega/smart_pot_mega.ino`](arduino/smart_pot_mega/smart_pot_mega.ino).

---

## Architecture

```
+----------------------+      Serial         +-----------------+   WiFi    +----------------+
|  Arduino Mega 2560   |  ---------------->  |    ESP8266      |  ------>  |  PHP backend   |
|                      |  "DATA,T,Ha,..."    |  HTTPClient     |  POST     |  Apache+MySQL  |
|  sensors + algorithm |                     |  JSON           |  JSON     |  + dashboard   |
+----------------------+                     +-----------------+           +----------------+
        |                                                                          ^
        v relay                                                                    |
   +---------+                                                              browser/phone
   |  pump   |
   +---------+
```

Wiring (Arduino Mega pinout):

| Component                  | Pin                                  |
|----------------------------|--------------------------------------|
| DHT22 (air T/RH)           | D7 (data), 5 V, GND                  |
| Soil moisture (analog)     | A2, 5 V, GND                         |
| Photoresistor              | A1 with 10 kOhm pull-down            |
| Rain sensor (analog)       | A0                                   |
| HC-SR04 ultrasonic         | D9 (TRIG), D10 (ECHO)                |
| Relay (pump control)       | D5, 5 V, GND                         |
| ESP8266 (TX/RX)            | Mega Serial1: D18 (TX1), D19 (RX1)   |

---

## Quick start

### 1. Backend

```bash
# clone
git clone https://github.com/<your-handle>/smart-pot-iot.git
cd smart-pot-iot

# create the database
mysql -u root -p < web/db.sql

# adjust DB credentials if needed
$EDITOR web/config.php

# serve - either with XAMPP / WAMP, or directly with PHP's built-in server:
php -S 0.0.0.0:8080 -t web
```

Open <http://localhost:8080>. A demo account is pre-seeded:

| field    | value             |
|----------|-------------------|
| username | `demo`            |
| password | `demo1234`        |
| code     | `DEMO-CODE-001`   |

### 2. Firmware

1. Open `arduino/smart_pot_mega/smart_pot_mega.ino` in the Arduino IDE,
   select **Arduino Mega or Mega 2560**, upload.
2. Open `arduino/esp8266_wifi/esp8266_wifi.ino`, set `WIFI_SSID`,
   `WIFI_PASSWORD`, `API_URL` and `POT_CODE`, then flash the ESP-01.
3. See [`arduino/libraries.md`](arduino/libraries.md) for required libraries.

### 3. Send a test reading without the hardware

```bash
curl -X POST -H 'Content-Type: application/json' \
     -d '{"code":"DEMO-CODE-001","temperature":24.1,"humidity":52,
          "soil":36,"light":680,"tank":0.85,"pump":0}' \
     http://localhost:8080/api/receive_data.php
```

Refresh the dashboard - the new value will be there.

---

## Repository layout

```
smart-pot-iot/
|-- arduino/
|   |-- smart_pot_mega/     # Mega firmware (sensors + algorithm + pump)
|   |-- esp8266_wifi/       # ESP8266 WiFi bridge
|   `-- libraries.md
|-- web/
|   |-- index.php           # redirect to login/dashboard
|   |-- register.php
|   |-- login.php
|   |-- logout.php
|   |-- dashboard.php       # live gauges + tank alert
|   |-- history.php
|   |-- config.php
|   |-- db.sql              # MySQL schema + demo seed
|   |-- api/
|   |   `-- receive_data.php
|   `-- assets/
|       `-- style.css
|-- docs/
|   `-- report.pdf          # original PFE memoir (add yours here)
|-- LICENSE
`-- README.md
```

---

## Note on this code

The original firmware and web sources were lost when the development laptop
failed. This repository is a faithful **reconstruction** based on the
published project report (pin assignments, formulas, UI screenshots and
flowcharts). It reproduces the documented behaviour, the Turc-based
algorithm and the original interface design - which means the system runs
end-to-end, but specific timings (sample interval, pump flow, soil
calibration constants) may need to be tuned for your hardware. Calibration
hints are in the comments at the top of each sketch.

---

## Team

- **Hannouni Salma**
- **Belassiri Mohamed Amine**
- **El Antary Jalal**

Under the supervision of:

- Pr. EL FILALI Sanae *(supervisor)*
- Pr. ELOUAHABI Sara *(co-supervisor)*
- Mr. TACE Youness *(co-supervisor)*
- Mme. ALAOUI Fatima Zahra *(co-supervisor)*

Defended **17 June 2022**, Faculte des Sciences Ben M'Sick, Universite
Hassan II de Casablanca.

---

## License

Released under the MIT License - see [LICENSE](LICENSE).
