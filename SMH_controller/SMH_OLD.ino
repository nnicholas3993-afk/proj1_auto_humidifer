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

// HARDCODED TIMEZONE OFFSET (East Coast US is -4 hours from UTC)
// -4 hours * 60 minutes * 60 seconds = -14400
const long utcOffsetInSeconds = -14400; 

// --- HARDCODED OFFLINE START TIME ---
// If Wi-Fi fails, the clock will start ticking from this EXACT time.
// Use 24-hour format (e.g., 14 for 2:00 PM, 0 for Midnight)
int offlineStartHour = 12;  
int offlineStartMinute = 0; 

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
  
  // 1. Initialize Relay (ACTIVE-HIGH LOGIC)
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW); // LOW = OFF
  
  // 2. Initialize Sensors & I2C
  dht.begin();
  Wire.begin(I2C_SDA, I2C_SCL); 
  
  // 3. Boot the OLED Screen
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("OLED allocation failed!"));
    for(;;); 
  }
  
  // 4. 3-Second Mister Test
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Booting System...");
  display.println("Testing Mister...");
  display.display();
  
  digitalWrite(RELAY_PIN, HIGH); // Turn ON mister
  delay(3000);                   // Hold for 3 seconds
  digitalWrite(RELAY_PIN, LOW);  // Turn OFF mister

  // 5. Connect to Wi-Fi with Loading Animation
  WiFi.mode(WIFI_STA); 
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) { // 10 second timeout
    display.clearDisplay();
    display.setCursor(0, 0);
    display.print("Connecting WiFi\n");
    
    // Animate the dots: 1, 2, 3, then reset
    int dots = (attempts % 4); 
    if (dots == 1) display.print(".");
    else if (dots == 2) display.print("..");
    else if (dots == 3) display.print("...");
    
    display.display();
    delay(500);
    attempts++;
  }
  
  // 6. Show Connection Status
  display.clearDisplay();
  display.setCursor(0,0);
  if (WiFi.status() == WL_CONNECTED) {
    display.println("WiFi Connected!");
    display.display();
    
    // Sync Atomic Time using hard mathematical offset
    configTime(utcOffsetInSeconds, 0, ntpServer);
    
    struct tm timeinfo;
    int timeAttempts = 0;
    while(!getLocalTime(&timeinfo) && timeAttempts < 10){
      delay(500); 
      timeAttempts++;
    }
  } else {
    display.println("WiFi Failed!");
    display.println("Using Hardcoded Time.");
    display.display();
    
    // Force the internal clock to the hardcoded time you set at the top
    // We add 1704067200 (Jan 1, 2024 in seconds) so the ESP32 doesn't reject it as a "1970 error"
    struct timeval tv;
    tv.tv_sec = 1704067200 + (offlineStartHour * 3600) + (offlineStartMinute * 60);
    tv.tv_usec = 0;
    settimeofday(&tv, NULL);
  }
  
  delay(3000); // Wait 3 seconds so you can read the status
}

void loop() {
  delay(2000); 
  
  // Fetch current time
  struct tm timeinfo;
  bool hasTime = getLocalTime(&timeinfo, 50);
  int currentHour = hasTime ? timeinfo.tm_hour : 0; 
  
  // Fetch climate data
  float humidity = dht.readHumidity();
  float tempF = dht.readTemperature(true);
  
  if (isnan(humidity) || isnan(tempF)) return;
  
  // --- AUTOMATION LOGIC (Crested Gecko Schedule) ---
  float targetHumidity = 50.0; 
  
  // 8:00 AM to 7:59 PM (Daytime) -> 50%
  if (currentHour >= 8 && currentHour < 20) {
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
  
  // --- RELAY CONTROL (Active-High) ---
  bool isMisterOn = false;
  
  if (humidity < targetHumidity) {
    digitalWrite(RELAY_PIN, HIGH); // ON
    isMisterOn = true;
  } 
  else if (humidity >= (targetHumidity + 5.0)) {
    digitalWrite(RELAY_PIN, LOW); // OFF
    isMisterOn = false;
  } 
  else {
    isMisterOn = (digitalRead(RELAY_PIN) == HIGH); 
  }
  
  // --- DRAW THE OLED DASHBOARD ---
  display.clearDisplay();
  
  // 1. Current Time (Top Left)
  display.setTextSize(1);
  display.setCursor(0, 0);
  if (hasTime) {
    int displayHour = timeinfo.tm_hour % 12;
    if (displayHour == 0) displayHour = 12; 
    String ampm = (timeinfo.tm_hour >= 12) ? "PM" : "AM";
    display.printf("%02d:%02d %s", displayHour, timeinfo.tm_min, ampm.c_str());
  } else {
    display.print("CLOCK ERROR");
  }
  
  // 2. Mist Maker Status (Top Right)
  display.setCursor(76, 0);
  if(isMisterOn) {
    display.print("MIST: ON");
  } else {
    display.print("MIST: OFF");
  }

  // Draw a horizontal line under the header
  display.drawLine(0, 10, 128, 10, SSD1306_WHITE);
  
  // 3. Live Temp (Left) & Humidity (Right) - Size 2
  display.setTextSize(2);
  display.setCursor(0, 20);
  display.printf("%.0fF", tempF);
  
  display.setCursor(70, 20);
  display.printf("%.0f%%", humidity);
  
  // 4. Target Humidity (Spanning Bottom)
  display.setTextSize(1);
  display.setCursor(0, 52);
  display.printf("TARGET HUMIDITY: %.0f%%", targetHumidity);
  
  display.display(); 
}
