# Domino4 Firmware - Sensor Integration Guide

## Overview
This firmware integrates the AGRIBOT system with Domino4 environmental sensors using I2C communication protocol.

## I2C Configuration
- **SDA (Serial Data):** GPIO 21
- **SCL (Serial Clock):** GPIO 22  
- **Bus Speed:** 100 kHz (for stability)
- **Voltage:** 3.3V

## Sensors

### 1. LTR390 (Light & UV Sensor)
**I2C Address:** `0x53` (85 decimal)

**Measurements:**
- Lux level (light intensity)
- UV index
- Ambient light detection

**Pinout:**
```
LTR390
├── VCC → 3.3V
├── GND → GND
├── SDA → GPIO 21
└── SCL → GPIO 22
```

**Specifications:**
- Supply voltage: 2.4V - 3.6V
- I2C frequency: 100-400 kHz
- Current draw: ~5 mA active

**Datasheet:** [LTR390 specs](https://optoelectronics.liteon.com/upload/download/DS86-2013-0005/LTR-390UV_PrelimDS_Ver5.pdf)

---

### 2. AHTxx (Humidity & Temperature Sensor)
**I2C Address:** `0x38` (56 decimal)

**Measurements:**
- Relative humidity (0-100%)
- Temperature (-20°C to +60°C)

**Pinout:**
```
AHTxx
├── VCC → 3.3V
├── GND → GND
├── SDA → GPIO 21
├── SCL → GPIO 22
└── ADR (optional pull to GND) → varies address
```

**Specifications:**
- Supply voltage: 2.2V - 5.5V (AHT2x)
- I2C speed: 100-400 kHz (10 kHz minimum)
- Accuracy: ±0.3°C, ±2% RH
- Measurement frequency: 8-30 seconds recommended

**Library:** `enjoyneering/AHTxx @ ^1.0.8`

---

### 3. SHT3x (Humidity & Temperature Sensor)
**I2C Address:** `0x44` (0x45 alternative)

**Measurements:**
- Relative humidity (0-100%)
- Temperature (-40°C to +125°C)

**Pinout:**
```
SHT3x
├── VCC → 3.3V
├── GND → GND
├── SDA → GPIO 21
└── SCL → GPIO 22
```

**Specifications:**
- Supply voltage: 2.4V - 5.5V
- I2C speed: 100-400 kHz
- Accuracy: ±2% RH, ±0.2°C
- Response time: < 8 seconds

**Library:** `wemos/WEMOS SHT3x Arduino Library @ ^1.0.0`

---

### 4. SPL06-007 (Barometer/Altimeter)
**I2C Address:** `0x76` (118 decimal)

**Measurements:**
- Atmospheric pressure (Pa)
- Altitude (calculated from pressure)
- Temperature

**Pinout:**
```
SPL06-007
├── VCC → 3.3V
├── GND → GND
├── SDA → GPIO 21
├── SCL → GPIO 22
├── EOC (optional) → not connected
├── INT (optional) → not connected
└── CSB (I2C mode) → GND
```

**Specifications:**
- Supply voltage: 1.65V - 3.6V
- I2C speed: 400 kHz
- Pressure range: 300-1100 hPa
- Pressure accuracy: ±50 Pa

**Library:** `rv701/SPL06-007 @ ^1.0.0`

---

### 5. SGP30 (Air Quality / TVOC Sensor)
**I2C Address:** `0x58` (88 decimal)

**Measurements:**
- CO2 equivalent (eCO2 in ppm)
- Total Volatile Organic Compounds (TVOC in ppb)

**Pinout:**
```
SGP30
├── VCC → 3.3V
├── GND → GND
├── SDA → GPIO 21
├── SCL → GPIO 22
├── SDA_PULL (optional) → 3.3V
└── SCL_PULL (optional) → 3.3V
```

**Specifications:**
- Supply voltage: 3.3V - 5.5V
- I2C speed: 100-400 kHz
- CO2 range: 400-60000 ppm
- TVOC range: 0-60000 ppb
- Warm-up time: 15 seconds

**Library:** `sparkfun/SparkFun SGP30 Arduino Library @ ^1.0.5`

---

### 6. ADC Sensors (Legacy Support)
These analog sensors connect to ESP32 ADC pins:

| Sensor | GPIO | Description |
|--------|------|-------------|
| Temperature | GPIO 32 | Analog temperature sensor |
| Humidity | GPIO 33 | Analog humidity sensor |
| Soil Moisture | GPIO 34 | Soil moisture capacitive sensor |
| pH | GPIO 35 | pH electrode via conditioning circuit |
| Battery Voltage | GPIO 39 | Battery voltage divider (2:1) |

**Wiring:**
```
ADC Sensor
├── Signal → GPIOxx (via potential divider if needed)
├── GND → GND
└── VCC → 3.3V (or sensor supply depending on circuit)
```

---

## Complete Wiring Diagram

```
┌─────────────────────────────────────────┐
│            ESP32 Dev Module             │
├─────────────────────────────────────────┤
│                                         │
│  GPIO 21 (SDA) ──────┬─────────────┐   │
│                      │             │   │
│  GPIO 22 (SCL) ──────┼─────────────┤   │
│                      │             │   │
│  GPIO 32 (ADC) ──→ Temperature    │   │
│  GPIO 33 (ADC) ──→ Humidity       │   │
│  GPIO 34 (ADC) ──→ Soil Moisture  │   │
│  GPIO 35 (ADC) ──→ pH             │   │
│  GPIO 39 (ADC) ──→ Battery Vol    │   │
│                                    │   │
│  GND ────────────────┬─────────────┤   │
│  3.3V ───────────────┼─────────────┤   │
│                      │             │   │
└──────────────────────┼─────────────┘   │
    ┌──────────────────┘                 │
    │                                    │
    ├─→ ┌──────────────┐                │
    │   │  LTR390      │                │
    │   │  Light/UV    │                │
    │   └──────────────┘                │
    │                                    │
    ├─→ ┌──────────────┐                │
    │   │  AHTxx       │                │
    │   │  Temp/Humid  │                │
    │   └──────────────┘                │
    │                                    │
    ├─→ ┌──────────────┐                │
    │   │   SHT3x      │                │
    │   │  Temp/Humid  │                │
    │   └──────────────┘                │
    │                                    │
    ├─→ ┌──────────────┐                │
    │   │  SPL06-007   │                │
    │   │  Barometer   │                │
    │   └──────────────┘                │
    │                                    │
    └─→ ┌──────────────┐                │
        │  SGP30       │                │
        │ Air Quality  │                │
        └──────────────┘                │
```

---

## API Response Format

### GET /sensors
```json
{
  "adc": {
    "temperatureC": 25.5,
    "humidityPct": 65.0,
    "soilMoisturePct": 45.3,
    "ph": 7.2,
    "batteryPct": 85.0
  },
  "domino4": {
    "light": {
      "luxLevel": 450.5,
      "uvIndex": 2.3
    },
    "airQuality": {
      "co2Ppm": 425,
      "tvocPpb": 50
    },
    "weather": {
      "temperatureC": 24.8,
      "humidityPct": 62.5,
      "pressurePa": 101325.0,
      "source": "AHTxx"
    }
  },
  "pins": {
    "temperature": 32,
    "humidity": 33,
    "soil": 34,
    "ph": 35,
    "battery": 39,
    "i2c": {
      "sda": 21,
      "scl": 22
    }
  },
  "systemInfo": {
    "i2cReady": true,
    "uptimeSeconds": 1234
  }
}
```

---

## Compilation & Upload

```bash
cd esp32-firmware
platformio run --target upload
```

**Board:** ESP32 Dev Module  
**Upload Speed:** 460800 baud  
**Monitor:** `platformio device monitor --baud 115200`

---

## Serial Output Example

```
AGRIBOT ESP32 firmware booting...
Initializing I2C bus...
I2C initialized: SDA=GPIO21, SCL=GPIO22
Scanning I2C bus for Domino4 sensors...
  Found I2C device at 0x38  (AHTxx)
  Found I2C device at 0x44  (SHT3x)
  Found I2C device at 0x53  (LTR390)
  Found I2C device at 0x58  (SGP30)
  Found I2C device at 0x76  (SPL06-007)
Total I2C devices found: 5
Initializing Domino4 I2C sensors...
✓ LTR390 (Light sensor) initialized
✓ SGP30 (Air quality) initialized
✓ AHTxx (Humidity/Temp) initialized
✓ SHT3x (Humidity/Temp) initialized
✓ SPL06-007 (Barometer) initialized
```

---

## Troubleshooting

### I2C Device Not Found
1. Check wiring (SDA/SCL to GPIO 21/22)
2. Verify 3.3V power supply
3. Check for pulled-up resistors (typically 4.7kΩ on SDA/SCL)
4. Ensure device address matches configuration

### Sensor Reading Errors
1. Run I2C bus scan at startup (enabled by default)
2. Check serial monitor for initialization messages
3. Verify sensor power supply voltage
4. Check pullup resistor values (should be 4.7kΩ)

### ADC Readings are 0
1. Verify ADC pin wiring
2. Check that sensors are providing voltage
3. Use GPIO 32-39 only (ADC1, safer with Wi-Fi)
4. Ensure proper signal conditioning circuit

---

## Key I2C Addresses Summary

| Sensor | Address (Hex) | Address (Dec) |
|--------|--------------|---------------|
| LTR390 | 0x53 | 83 |
| AHTxx | 0x38 | 56 |
| SHT3x | 0x44 | 68 |
| SPL06-007 | 0x76 | 118 |
| SGP30 | 0x58 | 88 |

---

## References

- [Arduino I2C Documentation](https://www.arduino.cc/reference/en/language/functions/communication/wire/)
- [ESP32 I2C User Guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/hw-reference/esp32_datasheet.pdf)
- [ArduinoJSON Library](https://arduinojson.org/)
- Individual sensor datasheets linked above
