# Smart Terrarium Climate Controller

An automated, ESP32-powered climate control system designed to maintain the perfect daily humidity cycle for a Crested Gecko. 

This system monitors live temperature and humidity, displays the data on an OLED dashboard, and automatically triggers a misting system based on a hardcoded daily schedule. If the Wi-Fi connection drops, the system gracefully falls back to an internal hardware clock to ensure the misting schedule never misses a beat.

## Features
* **Automated Daily Cycle:** 
  * 8:00 AM – 7:59 PM: 50% Target (Daytime dry-out to prevent respiratory issues)
  * 8:00 PM – 8:59 PM: 90% Target (Evening spike to simulate twilight dew)
  * 9:00 PM – 7:59 AM: 65% Target (Standard nighttime humidity)
* **OLED Dashboard:** Live display of current temp, humidity, target thresholds, and Wi-Fi/Time status.
* **Offline Resilience:** NTP server synchronization with an automatic localized offline clock fallback.
* **Boot Diagnostics:** Features an animated boot sequence and a 3-second hardware relay test blast.

## Bill of Materials (BOM)
* ESP32-C3 Super Mini
* DHT22 Temperature & Humidity Sensor
* 0.96" I2C OLED Display (SSD1306)
* 1-Channel Relay Module (Active-High)
* Mist Maker / Ultrasonic Humidifier
* Standard plastic water bottle (for the PCO-1881 reservoir)

## Wiring Guide
| Component | ESP32-C3 Pin |
| :--- | :--- |
| **DHT22 (Data)** | GPIO 2 |
| **Relay (IN/Signal)**| GPIO 3 |
| **OLED (SDA)** | GPIO 6 |
| **OLED (SCL)** | GPIO 7 |

*Note: The relay module must be wired to the NO (Normally Open) and COM terminals to ensure the mister defaults to OFF if the microcontroller loses power.*

## Enclosure & Hardware Fabrication
The `cad/` directory contains the FreeCAD files and STLs required for the gravity-fed water reservoir. 

The custom PCO-1881 threaded bottle neck receiver includes a specially engineered "notched" geometry at the bottom of the feed tube. This breaks the water's surface tension, allowing air to bypass the water line and preventing the vacuum air-lock commonly seen in narrow gravity feeders. These components are optimized for custom enclosures and can be successfully fabricated using either a fast filament workhorse like the Flashforge Adventurer 5M Pro or a high-detail resin machine like the Anycubic Photon Mono 4 depending on your watertight tolerance needs. 

## Software Dependencies
Install the following libraries via the Arduino IDE Library Manager:
* `Adafruit GFX Library`
* `Adafruit SSD1306`
* `DHT sensor library` (by Adafruit)

## Setup & Installation
1. Clone this repository.
2. Open `terrarium_controller.ino` in the Arduino IDE.
3. Update the `ssid` and `password` variables with your Wi-Fi credentials.
4. Flash to your ESP32-C3 Super Mini.
