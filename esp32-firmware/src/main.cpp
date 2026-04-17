#include <Arduino.h>
#include <ArduinoJson.h>
#include <WebServer.h>
#include <WiFi.h>
#include <Wire.h>
#include <math.h>

// Available Domino4 Sensor Libraries
#include <LTR390.h>         // Light sensor - ILB (0x53)
#include "WEMOS_SHT3X.h"    // Temperature/Humidity - IWA (0x44)
#include <Adafruit_SSD1306.h>  // OLED Display - ODA (0x3C)
#include <Adafruit_GFX.h>
#include "SparkFun_SGP30_Arduino_Library.h"  // Air quality (optional)

#if __has_include("secrets.h")
#include "secrets.h"
#endif

#ifndef WIFI_SSID
#define WIFI_SSID "AGRIBOT-ESP"
#endif

#ifndef WIFI_PASSWORD
#define WIFI_PASSWORD "agribot123"
#endif

#ifndef APP_NAME
#define APP_NAME "AGRIBOT-ESP"
#endif

namespace {
WebServer server(80);

// Domino4 I2C Pin Configuration
// Standard ESP32 I2C pins: GPIO21 (SDA), GPIO22 (SCL)
const int I2C_SDA_PIN = 21;
const int I2C_SCL_PIN = 22;

// Domino4 Sensor I2C Addresses
const uint8_t LTR390_ADDR = 0x53;      // ILB - Light sensor
const uint8_t SHT3X_ADDR = 0x44;       // IWA - Weather sensor (SHT3x)
const uint8_t OLED_ADDR = 0x3C;        // ODA - OLED Display
const uint8_t SGP30_ADDR = 0x58;       // IGA - Air quality (optional)

// Capacitive soil moisture on GPIO33 (T8) for CWV boards
const int SOIL_MOISTURE_PIN = 33;      // IWC - Soil moisture sensor

// Sensor objects
LTR390 light(LTR390_ADDR);
SHT3X sht30(SHT3X_ADDR);  // WEMOS SHT3x at address 0x44
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
SGP30 mysgp30;

// Sensor reading cache (to avoid re-reading too frequently)
struct {
  float lightLux = 0.0f;
  float lightUV = 0.0f;
  float tempC = 0.0f;
  float humidityPct = 0.0f;
  float soilMoisturePct = 0.0f;
  uint16_t co2 = 0;
  uint16_t tvoc = 0;
  bool i2cReady = false;
  bool sht3xReady = false;
  bool oledReady = false;
} sensorCache;

// Default ADC pin profile for ESP32-WROOM-32 (ADC1 pins are safer with Wi-Fi).
#ifndef SENSOR_PIN_TEMPERATURE
#define SENSOR_PIN_TEMPERATURE 32
#endif

#ifndef SENSOR_PIN_HUMIDITY
#define SENSOR_PIN_HUMIDITY 33
#endif

#ifndef SENSOR_PIN_SOIL
#define SENSOR_PIN_SOIL 34
#endif

#ifndef SENSOR_PIN_PH
#define SENSOR_PIN_PH 35
#endif

#ifndef SENSOR_PIN_BATTERY
#define SENSOR_PIN_BATTERY 39
#endif

float clampf(float value, float minValue, float maxValue) {
  if (value < minValue) {
    return minValue;
  }
  if (value > maxValue) {
    return maxValue;
  }
  return value;
}

// Forward declarations for ADC reading functions (used in various sensor reading functions)
int readAdcRaw(int pin);
float adcRawToVoltage(int raw);

// Initialize I2C bus
void initI2C() {
  Serial.println("[I2C] Initializing I2C bus...");
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  Wire.setClock(100000);  // 100kHz standard mode for SHT30 compatibility
  Wire.setTimeOut(1000);  // 1 second timeout to prevent hanging
  delay(100);
  Serial.printf("[I2C] Initialized: SDA=GPIO%d, SCL=GPIO%d, clock=100kHz, timeout=1000ms\n", I2C_SDA_PIN, I2C_SCL_PIN);
}

// Scan I2C bus for connected devices
void scanI2CBus() {
  Serial.println("[I2C] Scanning bus for devices...");
  int deviceCount = 0;
  for (uint8_t addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    byte error = Wire.endTransmission();
    if (error == 0) {
      Serial.printf("[I2C]   Found device at 0x%02X\n", addr);
      deviceCount++;
    }
  }
  Serial.printf("[I2C] Total devices found: %d\n", deviceCount);
  
  if (deviceCount == 0) {
    Serial.println("[I2C] WARNING: No I2C devices detected! Check wiring and power.");
    sensorCache.i2cReady = false;
    return;
  }
  sensorCache.i2cReady = true;
}

// Initialize Domino4 I2C sensors
void initializeDomino4Sensors() {
  if (!sensorCache.i2cReady) {
    Serial.println("[I2C] Skipping sensor init - no devices on bus");
    return;
  }
  
  Serial.println("[I2C] Initializing Domino4 sensors...");
  
  // Initialize LTR390 (Light/UV sensor) at 0x53
  if (light.init()) {
    Serial.println("[I2C]   LTR390 (Light/UV) initialized at 0x53");
    Serial.println("[I2C]     LTR390 configured: Ready for continuous light/UV readings");
  } else {
    Serial.println("[I2C]   LTR390 init failed at 0x53");
  }
  
  // Initialize SHT3x (Weather sensor) at 0x44
  // The WEMOS library handles I2C communication
  // SHT30-DIS requires explicit reset and setup
  Serial.println("[I2C] Initializing SHT3x (Temperature/Humidity) at 0x44...");
  
  // Send reset command to SHT30 (0x30A2 = soft reset)
  Wire.beginTransmission(SHT3X_ADDR);
  Wire.write(0x30);
  Wire.write(0xA2);
  if (Wire.endTransmission() == 0) {
    Serial.println("[I2C]   SHT3x reset command sent");
    delay(50);  // Wait for sensor to reset
  } else {
    Serial.println("[I2C]   SHT3x reset command FAILED - sensor may not be present");
  }
  
  sensorCache.sht3xReady = true;
  Serial.println("[I2C]   SHT3x (Temperature/Humidity) initialized at 0x44");
  Serial.println("[I2C]   SHT30-DIS: Sensirion sensor, I2C Standard Mode 100kHz");
  
  // Initialize OLED Display at 0x3C
  if (display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("[I2C]   OLED Display (SSD1306) initialized at 0x3C");
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("AGRIBOT ESP32");
    display.println("Sensors Ready");
    display.display();
    sensorCache.oledReady = true;
  } else {
    Serial.println("[I2C]   OLED Display init failed at 0x3C");
  }
  
  // SGP30 disabled - not detected on this board
  Serial.println("[I2C]   SGP30 (Air Quality) skipped - set ENABLE_SGP30=1 in platformio.ini to enable");
}

// Read all I2C sensors (with timeout guard)
void readDomino4Sensors() {
  if (!sensorCache.i2cReady) return;
  
  // Read LTR390 (Light/UV sensor) - continuous polling
  if (light.newDataAvailable()) {
    sensorCache.lightLux = light.getLux();   // Read ambient light in lux
    sensorCache.lightUV = light.getUVI();    // Read UV index
    
    Serial.printf("[LTR390] Lux: %.1f, UV Index: %.2f\n", sensorCache.lightLux, sensorCache.lightUV);
  } else {
    // Fallback: attempt force read if no interrupt pending
    // This ensures we get readings even if interrupt is missed
    static unsigned long lastForcedReadMs = 0;
    if (millis() - lastForcedReadMs >= 1000) {  // Force read every 1 second
      lastForcedReadMs = millis();
      sensorCache.lightLux = light.getLux();
      sensorCache.lightUV = light.getUVI();
      
      if (sensorCache.lightLux > 0 || sensorCache.lightUV > 0) {
        Serial.printf("[LTR390-FORCE] Lux: %.1f, UV Index: %.2f\n", sensorCache.lightLux, sensorCache.lightUV);
      }
    }
  }
  
  // Read SHT3x (Temperature/Humidity) 
  if (sensorCache.sht3xReady) {
    // WEMOS SHT3x library reads sensor and caches values
    // Trigger a read every 2 seconds
    static unsigned long lastSht3xReadMs = 0;
    if (millis() - lastSht3xReadMs >= 2000) {
      lastSht3xReadMs = millis();
      
      // Try WEMOS library first
      uint8_t sht_ret = sht30.get();  // Trigger sensor read
      
      if (sht_ret == 0) {
        // Successfully read sensor
        float readTemp = sht30.cTemp;
        float readHumid = sht30.humidity;
        
        // Validate readings
        if (readTemp > -50 && readTemp < 100 && readHumid >= 0 && readHumid <= 100) {
          sensorCache.tempC = readTemp;
          sensorCache.humidityPct = readHumid;
          
          Serial.printf("[SHT3x] Temp: %.2f°C, Humidity: %.1f%%\n", sensorCache.tempC, sensorCache.humidityPct);
        } else {
          Serial.printf("[SHT3x] Invalid reading: T=%.2f, H=%.1f - out of range\n", readTemp, readHumid);
        }
      } else {
        // WEMOS library failed - try direct I2C read
        Serial.printf("[SHT3x] Library read failed with code %d - trying direct I2C read\n", sht_ret);
        
        // Send measurement command: 0x2400 (Normal mode, no clock stretch)
        Wire.beginTransmission(SHT3X_ADDR);
        Wire.write(0x24);
        Wire.write(0x00);
        if (Wire.endTransmission() != 0) {
          Serial.println("[SHT3x] I2C transmission failed");
          return;
        }
        
        // Wait for measurement completion (~30ms)
        delay(40);
        
        // Read 6 bytes: 2 for temp, 1 CRC, 2 for humidity, 1 CRC
        Wire.requestFrom((uint8_t)SHT3X_ADDR, (uint8_t)6);
        if (Wire.available() >= 6) {
          uint8_t buf[6];
          for (int i = 0; i < 6; i++) {
            buf[i] = Wire.read();
          }
          
          // Parse temperature
          uint16_t tempRaw = (buf[0] << 8) | buf[1];
          float tempC_calc = -45.0f + 175.0f * (tempRaw / 65535.0f);
          
          // Parse humidity
          uint16_t humRaw = (buf[3] << 8) | buf[4];
          float humidity_calc = 100.0f * (humRaw / 65535.0f);
          
          // Validate and update cache
          if (tempC_calc > -50 && tempC_calc < 100 && humidity_calc >= 0 && humidity_calc <= 100) {
            sensorCache.tempC = tempC_calc;
            sensorCache.humidityPct = humidity_calc;
            Serial.printf("[SHT3x-I2C] Temp: %.2f°C, Humidity: %.1f%%\n", sensorCache.tempC, sensorCache.humidityPct);
          }
        } else {
          Serial.printf("[SHT3x] I2C read returned only %d bytes\n", Wire.available());
        }
      }
    }
  }
  
  // Read soil moisture via capacitive touch on GPIO33 (T8)
  // IWC - Input Weather C (Domino4) calibration
  // Since touchRead is not reliable on this board, use ADC from GPIO34 instead
  // Range: 0 to 4095 raw ADC value
  
  // Try touch sensor first
  uint32_t soilRawTouch = touchRead(SOIL_MOISTURE_PIN);
  
  // If touch sensor returns 0 consistently, fall back to ADC reading
  // This provides better compatibility on some boards
  static bool useTouchSensor = true;
  if (soilRawTouch == 0 && useTouchSensor) {
    // Try once more
    uint32_t retryTouch = touchRead(SOIL_MOISTURE_PIN);
    if (retryTouch == 0) {
      // Touch sensor not responding - fall back to ADC
      useTouchSensor = false;
      Serial.println("[SOIL] Touch sensor unresponsive - falling back to ADC reading");
    } else {
      soilRawTouch = retryTouch;
    }
  }
  
  // Debug output for soil sensor
  static unsigned long lastSoilDebugMs = 0;
  if (millis() - lastSoilDebugMs >= 5000) {
    lastSoilDebugMs = millis();
    if (useTouchSensor) {
      Serial.printf("[SOIL] Raw touch value: %lu (GPIO%d)\n", soilRawTouch, SOIL_MOISTURE_PIN);
    } else {
      int adcSoil = readAdcRaw(SENSOR_PIN_SOIL);
      Serial.printf("[SOIL] ADC raw value: %d (GPIO%d voltage: %.2fV)\n", adcSoil, SENSOR_PIN_SOIL, adcRawToVoltage(adcSoil));
    }
  }
  
  // Calculate soil moisture percentage
  if (useTouchSensor) {
    // Domino4 IWC calibration using touch sensor
    // Range: 20000 (dry) to 2300000 (very wet)
    if (soilRawTouch > 3000000) {
      sensorCache.soilMoisturePct = 0;  // Sensor not present or invalid
    } else if (soilRawTouch >= 20000) {
      // Valid range: 20000 (dry) to 2300000 (very wet)
      // Map using Domino4 calibration
      sensorCache.soilMoisturePct = map(soilRawTouch, 20000, 2300000, 0, 100);
      sensorCache.soilMoisturePct = constrain(sensorCache.soilMoisturePct, 0, 100);
    } else {
      sensorCache.soilMoisturePct = 0;  // Below minimum threshold
    }
  } else {
    // Use ADC reading from GPIO34 (SENSOR_PIN_SOIL)
    // Typical calibration: 0 ADC (dry) to ~3000+ ADC (wet)
    int adcSoil = readAdcRaw(SENSOR_PIN_SOIL);
    if (adcSoil < 0) {
      sensorCache.soilMoisturePct = 0;
    } else {
      // Map ADC range 0-4095 to percentage, assume 500 is dry, 3000+ is wet
      sensorCache.soilMoisturePct = map(adcSoil, 500, 3000, 0, 100);
      sensorCache.soilMoisturePct = constrain(sensorCache.soilMoisturePct, 0, 100);
    }
  }
  
  // SGP30 disabled - not detected on this board
}

void updateOLEDDisplay() {
  if (!sensorCache.oledReady) return;
  
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  
  // Line 1: Temperature + Humidity
  display.printf("T:%.1fC H:%.0f%%\n", sensorCache.tempC, sensorCache.humidityPct);
  
  // Line 2: Light sensor + UV
  display.printf("L:%.0flux UV:%.2f\n", sensorCache.lightLux, sensorCache.lightUV);
  
  // Line 3: Soil Moisture
  display.printf("Soil:%.0f%% ", sensorCache.soilMoisturePct);
  
  // WiFi status
  if (WiFi.status() == WL_CONNECTED) {
    display.print("WiFi_STA");
  } else if (WiFi.getMode() == WIFI_AP) {
    display.print("WiFi_AP");
  } else {
    display.print("NoWiFi");
  }
  
  display.printf("\nUptime:%lums", millis());
  
  display.display();
}

int readAdcRaw(int pin) {
  if (pin < 0) {
    return -1;
  }
  return analogRead(pin);
}

float adcRawToVoltage(int raw) {
  if (raw < 0) {
    return -1.0f;
  }
  return (raw / 4095.0f) * 3.3f;
}

void setupSensors() {
  Serial.println("[SETUP] Initializing sensor pins...");
  
  analogReadResolution(12);
  analogSetPinAttenuation(SENSOR_PIN_TEMPERATURE, ADC_11db);
  analogSetPinAttenuation(SENSOR_PIN_HUMIDITY, ADC_11db);
  analogSetPinAttenuation(SENSOR_PIN_SOIL, ADC_11db);
  analogSetPinAttenuation(SENSOR_PIN_PH, ADC_11db);
  analogSetPinAttenuation(SENSOR_PIN_BATTERY, ADC_11db);

  pinMode(SENSOR_PIN_TEMPERATURE, INPUT);
  pinMode(SENSOR_PIN_HUMIDITY, INPUT);
  pinMode(SENSOR_PIN_SOIL, INPUT);
  pinMode(SENSOR_PIN_PH, INPUT);
  pinMode(SENSOR_PIN_BATTERY, INPUT);
  
  // GPIO33 is touch pin T8 - initialize for capacitive touch reading
  Serial.printf("[SETUP] Touch sensor enabled on GPIO%d (T8)\\n", SOIL_MOISTURE_PIN);
  
  // Initialize I2C and Domino4 sensors
  initI2C();
  delay(500);
  scanI2CBus();
  delay(500);
  initializeDomino4Sensors();
  delay(500);
  
  Serial.println("[SETUP] All sensors initialized");
}

void printPinDiagnostics() {
  const int temperatureRaw = readAdcRaw(SENSOR_PIN_TEMPERATURE);
  const int humidityRaw = readAdcRaw(SENSOR_PIN_HUMIDITY);
  const int soilRaw = readAdcRaw(SENSOR_PIN_SOIL);
  const int phRaw = readAdcRaw(SENSOR_PIN_PH);
  const int batteryRaw = readAdcRaw(SENSOR_PIN_BATTERY);

  Serial.println("--- ESP32 Pin Diagnostics ---");
  Serial.printf("Temp   GPIO %d -> raw=%d voltage=%.3fV\n", SENSOR_PIN_TEMPERATURE, temperatureRaw, adcRawToVoltage(temperatureRaw));
  Serial.printf("Humidity GPIO %d -> raw=%d voltage=%.3fV\n", SENSOR_PIN_HUMIDITY, humidityRaw, adcRawToVoltage(humidityRaw));
  Serial.printf("Soil   GPIO %d -> raw=%d voltage=%.3fV\n", SENSOR_PIN_SOIL, soilRaw, adcRawToVoltage(soilRaw));
  Serial.printf("pH     GPIO %d -> raw=%d voltage=%.3fV\n", SENSOR_PIN_PH, phRaw, adcRawToVoltage(phRaw));
  Serial.printf("Battery GPIO %d -> raw=%d voltage=%.3fV\n", SENSOR_PIN_BATTERY, batteryRaw, adcRawToVoltage(batteryRaw));
  
  // Light sensor diagnostics
  Serial.println("--- I2C Sensor Diagnostics ---");
  Serial.printf("Light Sensor (LTR390) @ 0x53: Lux=%.1f, UV Index=%.2f\n", sensorCache.lightLux, sensorCache.lightUV);
  Serial.printf("Temperature/Humidity (SHT3x) @ 0x44: T=%.1f°C, H=%.0f%%\n", sensorCache.tempC, sensorCache.humidityPct);
  Serial.printf("Soil Moisture (Capacitive) @ GPIO33: %.0f%%\n", sensorCache.soilMoisturePct);
  Serial.printf("OLED Display @ 0x3C: %s\n", sensorCache.oledReady ? "Ready" : "Offline");
}

void addCorsHeaders() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET,OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
}

void sendJson(int statusCode, JsonDocument &doc) {
  String body;
  serializeJson(doc, body);
  addCorsHeaders();
  server.send(statusCode, "application/json", body);
}

void handleOptions() {
  addCorsHeaders();
  server.send(204);
}

void connectWifiOrStartAp() {
  if (String(WIFI_SSID).length() > 0) {
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.printf("Connecting to WiFi SSID: %s\n", WIFI_SSID);

    const unsigned long start = millis();
    const unsigned long timeoutMs = 15000;

    while (WiFi.status() != WL_CONNECTED && millis() - start < timeoutMs) {
      delay(300);
      Serial.print(".");
    }

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\nWiFi connected");
      Serial.print("ESP32 IP: ");
      Serial.println(WiFi.localIP());
      return;
    }

    Serial.println("\nWiFi connect timed out. Falling back to AP mode.");
  }

  WiFi.mode(WIFI_AP);
  const bool apOk = WiFi.softAP(APP_NAME, "agribot123");
  if (apOk) {
    Serial.print("AP started. SSID: ");
    Serial.println(APP_NAME);
    Serial.print("AP IP: ");
    Serial.println(WiFi.softAPIP());
  } else {
    Serial.println("Failed to start AP mode.");
  }
}

void handleHealth() {
  StaticJsonDocument<256> doc;
  doc["status"] = "ok";
  doc["name"] = APP_NAME;
  doc["ip"] = WiFi.getMode() == WIFI_AP ? WiFi.softAPIP().toString() : WiFi.localIP().toString();
  doc["uptimeMs"] = millis();
  doc["service"] = "sensor-data";
  sendJson(200, doc);
}

void handleSensors() {
  // Read all sensors (both I2C and ADC)
  readDomino4Sensors();
  
  const int tempRaw = readAdcRaw(SENSOR_PIN_TEMPERATURE);
  const int humidityRaw = readAdcRaw(SENSOR_PIN_HUMIDITY);
  const int soilRaw = readAdcRaw(SENSOR_PIN_SOIL);
  const int phRaw = readAdcRaw(SENSOR_PIN_PH);
  const int batteryRaw = readAdcRaw(SENSOR_PIN_BATTERY);

  const float tempVoltage = adcRawToVoltage(tempRaw);
  const float humidityVoltage = adcRawToVoltage(humidityRaw);
  const float soilVoltage = adcRawToVoltage(soilRaw);
  const float phVoltage = adcRawToVoltage(phRaw);
  const float batteryVoltagePin = adcRawToVoltage(batteryRaw);

  // Generic estimates; tune these formulas for your exact MaxIQ sensor models.
  const float temperature = tempVoltage < 0.0f ? -1.0f : clampf((tempVoltage - 0.5f) * 100.0f, -20.0f, 85.0f);
  const float humidity = humidityVoltage < 0.0f ? -1.0f : clampf((humidityVoltage / 3.0f) * 100.0f, 0.0f, 100.0f);
  const float soilMoisture = soilRaw < 0 ? -1.0f : clampf(100.0f - (soilRaw / 4095.0f) * 100.0f, 0.0f, 100.0f);
  const float ph = phVoltage < 0.0f ? -1.0f : clampf(7.0f + (2.5f - phVoltage) * 3.5f, 0.0f, 14.0f);

  // Assumes a 2:1 voltage divider from battery to ADC pin (adjust if different).
  const float batteryVoltage = batteryVoltagePin < 0.0f ? -1.0f : batteryVoltagePin * 2.0f;
  const float batteryPct = batteryVoltage < 0.0f ? -1.0f : clampf(((batteryVoltage - 3.3f) / (4.2f - 3.3f)) * 100.0f, 0.0f, 100.0f);

  DynamicJsonDocument doc(2048);
  
  // PRIMARY: Domino4 I2C Sensors (real-time, xChips standard)
  doc["domino4"]["weather"]["temperatureC"] = sensorCache.tempC;
  doc["domino4"]["weather"]["humidityPct"] = sensorCache.humidityPct;
  doc["domino4"]["weather"]["source"] = "SHT85 @ 0x44 (IWA)";
  
  doc["domino4"]["light"]["luxLevel"] = sensorCache.lightLux;
  doc["domino4"]["light"]["uvIndex"] = sensorCache.lightUV;
  doc["domino4"]["light"]["source"] = "LTR390 @ 0x53 (ILB)";
  
  doc["domino4"]["soil"]["moisturePct"] = sensorCache.soilMoisturePct;
  doc["domino4"]["soil"]["rawTouch"] = touchRead(SOIL_MOISTURE_PIN);
  doc["domino4"]["soil"]["source"] = "Capacitive Touch @ GPIO33 (IWC)";
  
  doc["domino4"]["display"]["active"] = sensorCache.oledReady;
  doc["domino4"]["display"]["source"] = "SSD1306 OLED @ 0x3C (ODA)";
  
  // LEGACY: ADC Sensors (kept for backward compatibility)
  doc["adc"]["temperatureC"] = temperature;
  doc["adc"]["humidityPct"] = humidity;
  doc["adc"]["soilMoisturePct"] = soilMoisture;
  doc["adc"]["ph"] = ph;
  doc["adc"]["batteryPct"] = batteryPct;
  doc["adc"]["note"] = "ADC values are optional/legacy. Use domino4 sensors for accurate readings.";
  
  // Optional: Air Quality (disabled by default)
  doc["airQuality"]["co2Ppm"] = sensorCache.co2;
  doc["airQuality"]["tvocPpb"] = sensorCache.tvoc;
  doc["airQuality"]["status"] = "SGP30 disabled - not present on this board";
  
  // GPS and camera (placeholder)
  doc["location"]["gps"]["lat"] = 5.6037;
  doc["location"]["gps"]["lng"] = -0.1870;
  doc["camera"]["streaming"] = false;

  // Raw ADC data
  doc["raw"]["temperatureAdc"] = tempRaw;
  doc["raw"]["humidityAdc"] = humidityRaw;
  doc["raw"]["soilAdc"] = soilRaw;
  doc["raw"]["phAdc"] = phRaw;
  doc["raw"]["batteryAdc"] = batteryRaw;

  // Voltages
  doc["voltage"]["temperatureV"] = tempVoltage;
  doc["voltage"]["humidityV"] = humidityVoltage;
  doc["voltage"]["soilV"] = soilVoltage;
  doc["voltage"]["phV"] = phVoltage;
  doc["voltage"]["batteryPinV"] = batteryVoltagePin;
  doc["voltage"]["batteryPackV"] = batteryVoltage;

  // Pin configuration
  doc["pins"]["temperature"] = SENSOR_PIN_TEMPERATURE;
  doc["pins"]["humidity"] = SENSOR_PIN_HUMIDITY;
  doc["pins"]["soil"] = SENSOR_PIN_SOIL;
  doc["pins"]["ph"] = SENSOR_PIN_PH;
  doc["pins"]["battery"] = SENSOR_PIN_BATTERY;
  doc["pins"]["i2c"]["sda"] = I2C_SDA_PIN;
  doc["pins"]["i2c"]["scl"] = I2C_SCL_PIN;
  doc["pins"]["soilMoistureTouch"] = SOIL_MOISTURE_PIN;

  // Backward compatibility: add top-level legacy fields
  doc["temperatureC"] = sensorCache.tempC > 0 ? sensorCache.tempC : temperature;
  doc["humidityPct"] = sensorCache.humidityPct > 0 ? sensorCache.humidityPct : humidity;
  doc["soilMoisturePct"] = sensorCache.soilMoisturePct >= 0 ? sensorCache.soilMoisturePct : soilMoisture;
  doc["ph"] = ph;
  doc["batteryPct"] = batteryPct;

  doc["systemInfo"]["i2cReady"] = sensorCache.i2cReady;
  doc["systemInfo"]["sht3xReady"] = sensorCache.sht3xReady;
  doc["systemInfo"]["oledReady"] = sensorCache.oledReady;
  doc["systemInfo"]["uptimeSeconds"] = millis() / 1000;
  
  sendJson(200, doc);
}

void handlePins() {
  StaticJsonDocument<768> doc;
  const int temperatureRaw = readAdcRaw(SENSOR_PIN_TEMPERATURE);
  const int humidityRaw = readAdcRaw(SENSOR_PIN_HUMIDITY);
  const int soilRaw = readAdcRaw(SENSOR_PIN_SOIL);
  const int phRaw = readAdcRaw(SENSOR_PIN_PH);
  const int batteryRaw = readAdcRaw(SENSOR_PIN_BATTERY);

  doc["pins"]["temperature"] = SENSOR_PIN_TEMPERATURE;
  doc["pins"]["humidity"] = SENSOR_PIN_HUMIDITY;
  doc["pins"]["soil"] = SENSOR_PIN_SOIL;
  doc["pins"]["ph"] = SENSOR_PIN_PH;
  doc["pins"]["battery"] = SENSOR_PIN_BATTERY;

  doc["readings"]["temperatureRaw"] = temperatureRaw;
  doc["readings"]["humidityRaw"] = humidityRaw;
  doc["readings"]["soilRaw"] = soilRaw;
  doc["readings"]["phRaw"] = phRaw;
  doc["readings"]["batteryRaw"] = batteryRaw;

  doc["readings"]["temperatureV"] = adcRawToVoltage(temperatureRaw);
  doc["readings"]["humidityV"] = adcRawToVoltage(humidityRaw);
  doc["readings"]["soilV"] = adcRawToVoltage(soilRaw);
  doc["readings"]["phV"] = adcRawToVoltage(phRaw);
  doc["readings"]["batteryV"] = adcRawToVoltage(batteryRaw);

  doc["howToCheck"] = "Disconnect one sensor at a time and verify its raw ADC value changes. Use only safe ADC levels on ESP32 GPIO32-39.";

  sendJson(200, doc);
}

void handleRoot() {
  StaticJsonDocument<384> doc;
  doc["name"] = APP_NAME;
  doc["routes"][0] = "/health (GET)";
  doc["routes"][1] = "/sensors (GET)";
  doc["routes"][2] = "/pins (GET)";
  sendJson(200, doc);
}

void setupHttpApi() {
  server.on("/", HTTP_GET, handleRoot);
  server.on("/health", HTTP_GET, handleHealth);
  server.on("/sensors", HTTP_GET, handleSensors);
  server.on("/pins", HTTP_GET, handlePins);
  server.onNotFound([]() {
    if (server.method() == HTTP_OPTIONS) {
      handleOptions();
      return;
    }

    StaticJsonDocument<128> doc;
    doc["error"] = "Not found";
    sendJson(404, doc);
  });

  server.begin();
  Serial.println("HTTP API started on port 80");
}
}  // namespace

void setup() {
  Serial.begin(115200);
  delay(200);

  Serial.println();
  Serial.println("========== AGRIBOT ESP32 BOOTING ==========");
  Serial.printf("Uptime: 0ms\n");

  setupSensors();
  
  // Trigger first sensor reads
  delay(500);
  readDomino4Sensors();
  delay(500);
  
  connectWifiOrStartAp();
  setupHttpApi();
  printPinDiagnostics();
  
  Serial.println("========== AGRIBOT ESP32 READY ===========");
}

void loop() {
  server.handleClient();

  // Read sensors every 2 seconds
  static unsigned long lastSensorReadMs = 0;
  if (millis() - lastSensorReadMs >= 2000) {
    lastSensorReadMs = millis();
    readDomino4Sensors();
    updateOLEDDisplay();  // Update display with new sensor data
  }

  // Print diagnostics every 5 seconds
  static unsigned long lastDiagnosticsMs = 0;
  if (millis() - lastDiagnosticsMs >= 5000) {
    lastDiagnosticsMs = millis();
    printPinDiagnostics();
  }
}
