# MotoSleep ESP32 Controller

A standalone ESP32 firmware for controlling MotoSleep adjustable beds via Bluetooth Low Energy (BLE), with Home Assistant integration through MQTT.

## Features

- **Direct BLE Control**: Connects directly to MotoSleep bed controllers
- **Multi-Bed Support**: Control multiple beds from a single ESP32
- **Home Assistant Integration**: Auto-discovery via MQTT
- **All Bed Features**:
  - Motor controls (Head, Feet, Neck/Lumbar)
  - Presets (Home/Flat, Memory 1 & 2, Anti-Snore, TV, Zero Gravity)
  - Preset programming (save current position)
  - Massage controls (Head, Feet, intensity cycling)
  - Under-bed lighting

## Hardware Requirements

- ESP32 development board (ESP32-WROOM-32 recommended)
- USB cable for programming
- Power supply (USB or 5V)

## Software Requirements

- [PlatformIO](https://platformio.org/) (VS Code extension or CLI)
- MQTT broker (e.g., Mosquitto in Home Assistant)

## Installation

### 1. Clone the Repository

```bash
git clone https://github.com/yourusername/motosleep-esp32.git
cd motosleep-esp32
```

### 2. Configure

Copy the template configuration file:

```bash
cp include/config.h.template include/config.h
```

Edit `include/config.h` with your settings:

```cpp
// WiFi
#define WIFI_SSID "YourWiFiNetwork"
#define WIFI_PASSWORD "YourWiFiPassword"

// MQTT
#define MQTT_HOST "homeassistant.local"
#define MQTT_PORT 1883
#define MQTT_USER "mqtt_user"        // Leave empty if no auth
#define MQTT_PASSWORD "mqtt_pass"    // Leave empty if no auth

// Your beds (find the BLE name using a BLE scanner app)
const BedConfig BEDS[] = {
    {"HHC1234567890", "Bedroom Bed", "bedroom"},
    {"HHC0987654321", "Guest Bed", "guest"},
};
```

### 3. Build and Upload

Using PlatformIO CLI:
```bash
pio run -t upload
```

Or use the PlatformIO IDE upload button in VS Code.

### 4. Monitor (Optional)

```bash
pio device monitor
```

## Finding Your Bed's BLE Name

Your MotoSleep bed broadcasts a BLE name starting with "HHC" followed by numbers and letters. To find it:

1. Use a BLE scanner app on your phone (e.g., "nRF Connect" or "LightBlue")
2. Look for devices starting with "HHC"
3. The name will be something like `HHC3611243CDEF`

## Home Assistant Setup

### Prerequisites

1. MQTT broker installed and configured (Mosquitto addon recommended)
2. MQTT integration enabled in Home Assistant

### Auto-Discovery

Once the ESP32 is running and connected to MQTT, entities will automatically appear in Home Assistant under the configured device names.

### Entities Created

For each bed, the following button entities are created:

**Motor Controls:**
- Head Up / Head Down
- Feet Up / Feet Down
- Neck Up / Neck Down

**Presets:**
- Flat/Home
- Memory 1 / Memory 2
- Anti-Snore
- TV
- Zero Gravity

**Preset Programming:**
- Save Memory 1 / Save Memory 2
- Save Anti-Snore
- Save TV
- Save Zero Gravity

**Massage:**
- Head Massage (cycles intensity)
- Foot Massage (cycles intensity)
- Head Massage Off
- Foot Massage Off
- Stop All

**Lighting:**
- Under-Bed Lights Toggle

## MQTT Topics

### Command Topics
```
motosleep/{bed_id}/{command}/set
```

Example:
```
motosleep/master_left/preset_zero_g/set
```

### Status Topic
```
motosleep/status
```
Values: `online` or `offline`

## Troubleshooting

### Bed Not Found
- Ensure the bed is powered on
- Check the BLE name matches exactly (case-sensitive)
- Move the ESP32 closer to the bed
- Check serial monitor for scan results

### MQTT Connection Failed
- Verify MQTT broker is running
- Check credentials in config.h
- Ensure the ESP32 is on the same network

### Commands Not Working
- Check serial monitor for error messages
- Verify bed is discovered (check logs)
- Some beds only allow one BLE connection - ensure the app is closed

## Protocol Reference

MotoSleep beds use a simple 2-byte command protocol:
- Byte 1: `0x24` (ASCII `$`)
- Byte 2: Command character (e.g., `K` for head up)

BLE Service: `0000ffe0-0000-1000-8000-00805f9b34fb`
BLE Characteristic: `0000ffe1-0000-1000-8000-00805f9b34fb`

## License

MIT License - See LICENSE file for details.

## Credits

Protocol reverse-engineered from the [smartbed-mqtt](https://github.com/richardhopton/smartbed-mqtt) project.
