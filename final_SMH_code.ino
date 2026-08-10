#include <WiFi.h>
#include "time.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "DHT.h"

// --- WIFI & TIME SETTINGS ---
const char* ssid       = "NETGEAR53";
const char* password   = "excitedmint952";
const char* ntpServer  = "pool.ntp.org";
const char* time_zone  = "EST5EDT,M3.2.0,M11.1.0"; // US Eastern Time

// --- HARDWARE PINS (ESP32-C3 Super Mini) ---
#define DHTPIN 2
#define DHTTYPE DHT22
#define RELAY_PIN 3    // Connect Relay IN to GPIO 3
#define I2C_SDA 6      // Connect OLED SDA to GPIO 6
#define I2C_SCL 7      // Connect OLED SCL to GPIO 7

// --- OLED SETUP ---
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1 
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

DHT dht(DHTPIN, DHTTYPE);

void setup() {
  Serial.begin(115200);
  
  // 1. Initialize Relay (ACTIVE-LOW LOGIC)
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH); // Sending HIGH turns an active-low relay OFF
  
  // 2. Initialize Sensors & I2C
  dht.begin();
  Wire.begin(I2C_SDA, I2C_SCL); 
  
  // 3. Boot the OLED Screen
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("OLED allocation failed! Check wiring."));
    for(;;); // Halt the system if screen fails
  }
  
  // Show boot sequence on screen
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  display.println("Booting System...");
  display.display();

  // 4. Connect to Wi-Fi (WITH TIMEOUT PROTECTION)
  WiFi.mode(WIFI_STA); // Force Station mode to prevent AP conflicts
  WiFi.begin(ssid, password);
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    display.print(".");
    display.display();
    attempts++;
    if(attempts % 10 == 0){
       display.println(); // Drop to a new line if it takes a while
    }
  }
  
  display.clearDisplay();
  display.setCursor(0,0);
  if (WiFi.status() == WL_CONNECTED) {
    display.println("WiFi Connected!");
  } else {
    display.println("WiFi Failed! Offline.");
  }
  display.display();

  // 5. Sync Atomic Time (ONLY IF CONNECTED)
  if (WiFi.status() == WL_CONNECTED) {
    configTzTime(time_zone, ntpServer);
    display.println("Syncing clock...");
    display.display();
    
    struct tm timeinfo;
    int timeAttempts = 0;
    while(!getLocalTime(&timeinfo) && timeAttempts < 10){
      delay(1000); // Wait until time is retrieved
      timeAttempts++;
    }
  }
  
  delay(2000); // Let the user read the status screen
}

void loop() {
  delay(2000); // Sensor hardware limits to 1 read every 2 seconds
  
  // Fetch current time safely (50ms timeout so it doesn't freeze)
  struct tm timeinfo;
  bool hasTime = getLocalTime(&timeinfo, 50);
  int currentHour = hasTime ? timeinfo.tm_hour : -1;
  
  // Fetch climate data
  float humidity = dht.readHumidity();
  float tempF = dht.readTemperature(true);
  
  // Error handling
  if (isnan(humidity) || isnan(tempF)) {
    Serial.println("DHT Sensor Error");
    return;
  }
  
  // --- AUTOMATION LOGIC (Custom Schedule) ---
  float targetHumidity = 50.0; // Default fallback
  
  if (!hasTime) {
    // OFFLINE FALLBACK: If router dies, safely hold 50% 24/7
    targetHumidity = 50.0; 
  }
  // 8:00 AM to 7:59 PM (Daytime) -> 50%
  else if (currentHour >= 8 && currentHour < 20) {
    targetHumidity = 50.0;
  } 
  // 8:00 PM to 8:59 PM (Evening Spike) -> 90%
  else if (currentHour == 20) {
    targetHumidity = 90.0;
  } 
  // 9:00 PM to 7:59 AM (Nighttime) -> 65%
  else if (currentHour >= 21 || currentHour < 8) {
    targetHumidity = 65.0;
  }
  
  // --- RELAY CONTROL (Active-Low Fix) ---
  bool isMisterOn = false;
  
  if (humidity < targetHumidity) {
    digitalWrite(RELAY_PIN, LOW); // LOW turns the relay ON
    isMisterOn = true;
  } 
  else if (humidity >= (targetHumidity + 5.0)) { // 5% deadband to stop rapid clicking
    digitalWrite(RELAY_PIN, HIGH); // HIGH turns the relay OFF
    isMisterOn = false;
  } 
  else {
    // If hovering in the deadband gap, check the actual pin state to maintain current status
    isMisterOn = (digitalRead(RELAY_PIN) == LOW); 
  }
  
  // --- DRAW THE OLED DASHBOARD ---
  display.clearDisplay();
  
  if (hasTime) {
    // Format the clock for 12-hour AM/PM
    int displayHour = timeinfo.tm_hour % 12;
    if (displayHour == 0) displayHour = 12; // Midnight and Noon should be 12, not 0
    String ampm = (timeinfo.tm_hour >= 12) ? "PM" : "AM";
    
    // 1. Current Time (Top Left)
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.printf("%02d:%02d %s", displayHour, timeinfo.tm_min, ampm.c_str());
  } else {
    // Show Offline Status if internet is down
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.print("OFFLINE");
  }
  
  // 2. Mist Maker Status (Top Right)
  display.setTextSize(1);
  display.setCursor(76, 0);
  if(isMisterOn) {
    display.print("MIST: ON");
  } else {
    display.print("MIST: OFF");
  }

  // Draw a horizontal line under the header
  display.drawLine(0, 10, 128, 10, SSD1306_WHITE);
  
  // 3. Live Humidity (Center Large)
  display.setTextSize(2);
  display.setCursor(0, 20);
  display.printf("Hum: %.1f%%", humidity);
  
  // 4. Live Temp & Current Target (Bottom Row)
  display.setTextSize(1);
  display.setCursor(0, 50);
  display.printf("T:%.1fF | Tgt:%.0f%%", tempF, targetHumidity);
  
  // Push the drawing buffer to the physical screen
  display.display(); 
}