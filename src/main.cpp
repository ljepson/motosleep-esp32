#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_mac.h>
#include <PubSubClient.h>
#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

#include "config.h"
#include "MotoSleepCommands.h"
#include "MotoSleepBed.h"
#include "HADiscovery.h"

// =============================================================================
// Global Objects
// =============================================================================
WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);
HADiscovery* haDiscovery = nullptr;
BLEScan* bleScan = nullptr;

// Array of bed objects
MotoSleepBed* beds[BED_COUNT];
bool bedsDiscovered[BED_COUNT] = {false};

// Timing
unsigned long lastMqttReconnect = 0;
unsigned long lastBleScan = 0;
bool allBedsFound = false;

// =============================================================================
// BLE Scan Callback
// =============================================================================
class ScanCallback : public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) override {
        String name = advertisedDevice.getName().c_str();

        if (name.length() == 0) return;

        // Check if this device matches any of our configured beds
        for (size_t i = 0; i < BED_COUNT; i++) {
            if (!bedsDiscovered[i] && name.equals(BEDS[i].bleName)) {
                Serial.printf("[BLE] Found bed: %s at %s\n",
                    BEDS[i].friendlyName,
                    advertisedDevice.getAddress().toString().c_str());

                beds[i]->setAddress(new BLEAddress(advertisedDevice.getAddress()));
                bedsDiscovered[i] = true;

                // Check if all beds are found
                allBedsFound = true;
                for (size_t j = 0; j < BED_COUNT; j++) {
                    if (!bedsDiscovered[j]) {
                        allBedsFound = false;
                        break;
                    }
                }

                if (allBedsFound) {
                    Serial.println("[BLE] All beds found, stopping scan");
                    bleScan->stop();
                }
                break;
            }
        }
    }
};

// =============================================================================
// MQTT Callback
// =============================================================================
void mqttCallback(char* topic, byte* payload, unsigned int length) {
    // Null-terminate payload
    char message[length + 1];
    memcpy(message, payload, length);
    message[length] = '\0';

    Serial.printf("[MQTT] Received: %s = %s\n", topic, message);

    // Parse topic: motosleep/{bed_id}/{command}/set
    String topicStr = String(topic);

    if (!topicStr.startsWith("motosleep/")) return;

    // Extract bed_id and command
    int firstSlash = topicStr.indexOf('/', 10);  // After "motosleep/"
    int secondSlash = topicStr.indexOf('/', firstSlash + 1);

    if (firstSlash == -1 || secondSlash == -1) return;

    String bedId = topicStr.substring(10, firstSlash);
    String command = topicStr.substring(firstSlash + 1, secondSlash);

    // Find the bed
    MotoSleepBed* targetBed = nullptr;
    for (size_t i = 0; i < BED_COUNT; i++) {
        if (bedId.equals(beds[i]->getId())) {
            targetBed = beds[i];
            break;
        }
    }

    if (!targetBed) {
        Serial.printf("[MQTT] Unknown bed ID: %s\n", bedId.c_str());
        return;
    }

    if (!targetBed->hasAddress()) {
        Serial.printf("[MQTT] Bed %s not discovered yet\n", bedId.c_str());
        return;
    }

    // Find the command
    char cmdChar = 0;

    // Check motor commands
    for (size_t i = 0; i < MotoSleep::MOTOR_COMMAND_COUNT; i++) {
        if (command.equals(MotoSleep::MOTOR_COMMANDS[i].name)) {
            cmdChar = MotoSleep::MOTOR_COMMANDS[i].cmdChar;
            break;
        }
    }

    // Check preset commands
    if (!cmdChar) {
        for (size_t i = 0; i < MotoSleep::PRESET_COMMAND_COUNT; i++) {
            if (command.equals(MotoSleep::PRESET_COMMANDS[i].name)) {
                cmdChar = MotoSleep::PRESET_COMMANDS[i].cmdChar;
                break;
            }
        }
    }

    // Check program commands
    if (!cmdChar) {
        for (size_t i = 0; i < MotoSleep::PROGRAM_COMMAND_COUNT; i++) {
            if (command.equals(MotoSleep::PROGRAM_COMMANDS[i].name)) {
                cmdChar = MotoSleep::PROGRAM_COMMANDS[i].cmdChar;
                break;
            }
        }
    }

    // Check massage commands
    if (!cmdChar) {
        for (size_t i = 0; i < MotoSleep::MASSAGE_COMMAND_COUNT; i++) {
            if (command.equals(MotoSleep::MASSAGE_COMMANDS[i].name)) {
                cmdChar = MotoSleep::MASSAGE_COMMANDS[i].cmdChar;
                break;
            }
        }
    }

    // Check light commands
    if (!cmdChar) {
        for (size_t i = 0; i < MotoSleep::LIGHT_COMMAND_COUNT; i++) {
            if (command.equals(MotoSleep::LIGHT_COMMANDS[i].name)) {
                cmdChar = MotoSleep::LIGHT_COMMANDS[i].cmdChar;
                break;
            }
        }
    }

    if (!cmdChar) {
        Serial.printf("[MQTT] Unknown command: %s\n", command.c_str());
        return;
    }

    // Send the command
    Serial.printf("[MQTT] Sending command '%c' to bed %s\n", cmdChar, targetBed->getFriendlyName());
    targetBed->sendCommand(cmdChar);
}

// =============================================================================
// WiFi Event Handler
// =============================================================================
void onWiFiEvent(WiFiEvent_t event, WiFiEventInfo_t info) {
    Serial.printf("[WiFi Event] ");
    switch(event) {
        case ARDUINO_EVENT_WIFI_STA_START:
            Serial.println("STA Started");
            break;
        case ARDUINO_EVENT_WIFI_STA_CONNECTED:
            Serial.println("Connected to AP");
            break;
        case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
            Serial.printf("Disconnected. Reason: %d\n", info.wifi_sta_disconnected.reason);
            break;
        case ARDUINO_EVENT_WIFI_STA_GOT_IP:
            Serial.printf("Got IP: %s\n", IPAddress(info.got_ip.ip_info.ip.addr).toString().c_str());
            break;
        case ARDUINO_EVENT_WIFI_STA_LOST_IP:
            Serial.println("Lost IP");
            break;
        default:
            Serial.printf("Event: %d\n", event);
            break;
    }
}

// =============================================================================
// WiFi Setup
// =============================================================================
void printWiFiStatus(wl_status_t status) {
    switch(status) {
        case WL_IDLE_STATUS: Serial.print("IDLE"); break;
        case WL_NO_SSID_AVAIL: Serial.print("NO_SSID_AVAIL"); break;
        case WL_SCAN_COMPLETED: Serial.print("SCAN_COMPLETED"); break;
        case WL_CONNECTED: Serial.print("CONNECTED"); break;
        case WL_CONNECT_FAILED: Serial.print("CONNECT_FAILED"); break;
        case WL_CONNECTION_LOST: Serial.print("CONNECTION_LOST"); break;
        case WL_DISCONNECTED: Serial.print("DISCONNECTED"); break;
        default: Serial.printf("UNKNOWN(%d)", status); break;
    }
}

// Static IP configuration - set USE_STATIC_IP to true and configure addresses
#define USE_STATIC_IP true
#define STATIC_IP      192, 168, 1, 200   // Change to match your network
#define STATIC_GATEWAY 192, 168, 1, 1     // Your router IP
#define STATIC_SUBNET  255, 255, 255, 0
#define STATIC_DNS     8, 8, 8, 8         // Google DNS

void setupWiFi() {
    Serial.printf("[WiFi] Connecting to: %s\n", WIFI_SSID);
    Serial.printf("[WiFi] Password length: %d\n", strlen(WIFI_PASSWORD));

    // Register event handler first
    WiFi.onEvent(onWiFiEvent);

    // Initialize WiFi
    WiFi.mode(WIFI_STA);
    delay(100);

    // Now read MAC (after WiFi is initialized)
    Serial.printf("[WiFi] ESP32 MAC: %s\n", WiFi.macAddress().c_str());

    #if USE_STATIC_IP
    IPAddress staticIP(STATIC_IP);
    IPAddress gateway(STATIC_GATEWAY);
    IPAddress subnet(STATIC_SUBNET);
    IPAddress dns(STATIC_DNS);

    Serial.printf("[WiFi] Using static IP: %s\n", staticIP.toString().c_str());
    if (!WiFi.config(staticIP, gateway, subnet, dns)) {
        Serial.println("[WiFi] Static IP config failed!");
    }
    #else
    Serial.println("[WiFi] Using DHCP");
    #endif

    // Disable power saving
    esp_wifi_set_ps(WIFI_PS_NONE);

    for (int retry = 1; retry <= 3; retry++) {
        Serial.printf("\n[WiFi] Attempt %d of 3\n", retry);

        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

        // Wait up to 20 seconds for connection (DHCP can be slow)
        unsigned long startTime = millis();
        while (WiFi.status() != WL_CONNECTED && (millis() - startTime) < 20000) {
            delay(500);
            Serial.print(".");

            // Check if we got an IP even if status doesn't update
            if (WiFi.localIP() != IPAddress(0, 0, 0, 0)) {
                Serial.println("\n[WiFi] Got IP!");
                break;
            }
        }
        Serial.println();

        if (WiFi.status() == WL_CONNECTED || WiFi.localIP() != IPAddress(0, 0, 0, 0)) {
            Serial.println("[WiFi] SUCCESS!");
            Serial.printf("[WiFi] IP: %s\n", WiFi.localIP().toString().c_str());
            Serial.printf("[WiFi] Gateway: %s\n", WiFi.gatewayIP().toString().c_str());
            Serial.printf("[WiFi] RSSI: %d dBm\n", WiFi.RSSI());
            Serial.printf("[WiFi] MAC: %s\n", WiFi.macAddress().c_str());
            return;
        }

        Serial.printf("[WiFi] Failed. Status: ");
        printWiFiStatus(WiFi.status());
        Serial.printf(" | IP: %s\n", WiFi.localIP().toString().c_str());

        WiFi.disconnect(true);
        delay(3000);
    }

    Serial.println("[WiFi] All attempts failed. Restarting...");
    delay(5000);
    ESP.restart();
}

// =============================================================================
// MQTT Setup
// =============================================================================
void setupMQTT() {
    mqtt.setServer(MQTT_HOST, MQTT_PORT);
    mqtt.setCallback(mqttCallback);
    mqtt.setBufferSize(1024);  // Larger buffer for discovery payloads
}

bool connectMQTT() {
    if (mqtt.connected()) return true;

    Serial.println("[MQTT] Connecting...");

    String clientId = DEVICE_NAME;
    clientId += "_";
    clientId += String(random(0xffff), HEX);

    // Connect with last will
    if (mqtt.connect(clientId.c_str(), MQTT_USER, MQTT_PASSWORD,
                     "motosleep/status", 0, true, "offline")) {
        Serial.println("[MQTT] Connected!");

        // Publish online status
        mqtt.publish("motosleep/status", "online", true);

        // Subscribe to command topics for all beds
        for (size_t i = 0; i < BED_COUNT; i++) {
            String topic = "motosleep/";
            topic += BEDS[i].id;
            topic += "/+/set";
            mqtt.subscribe(topic.c_str());
            Serial.printf("[MQTT] Subscribed to: %s\n", topic.c_str());
        }

        // Publish HA discovery
        haDiscovery->publishControllerDiscovery();
        for (size_t i = 0; i < BED_COUNT; i++) {
            haDiscovery->publishBedDiscovery(BEDS[i]);
        }

        return true;
    } else {
        Serial.printf("[MQTT] Failed, rc=%d\n", mqtt.state());
        return false;
    }
}

// =============================================================================
// BLE Setup
// =============================================================================
void setupBLE() {
    Serial.println("[BLE] Initializing...");
    BLEDevice::init(DEVICE_NAME);

    bleScan = BLEDevice::getScan();
    bleScan->setAdvertisedDeviceCallbacks(new ScanCallback());
    bleScan->setActiveScan(true);
    bleScan->setInterval(100);
    bleScan->setWindow(99);
}

void startBleScan() {
    if (allBedsFound) return;

    Serial.println("[BLE] Starting scan...");
    bleScan->start(BLE_SCAN_DURATION, false);
    lastBleScan = millis();
}

// =============================================================================
// Setup
// =============================================================================
void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println();
    Serial.println("================================");
    Serial.println("  MotoSleep ESP32 Controller");
    Serial.println("================================");
    Serial.printf("Configured beds: %d\n", BED_COUNT);
    for (size_t i = 0; i < BED_COUNT; i++) {
        Serial.printf("  - %s (%s)\n", BEDS[i].friendlyName, BEDS[i].bleName);
    }
    Serial.println();

    // Initialize bed objects
    for (size_t i = 0; i < BED_COUNT; i++) {
        beds[i] = new MotoSleepBed(BEDS[i]);
    }

    // Setup components
    setupWiFi();
    setupMQTT();
    setupBLE();

    // Create HA discovery helper
    haDiscovery = new HADiscovery(mqtt);

    // Initial MQTT connection
    connectMQTT();

    // Start initial BLE scan
    startBleScan();
}

// =============================================================================
// Loop
// =============================================================================
void loop() {
    // Maintain WiFi connection
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[WiFi] Connection lost, reconnecting...");
        setupWiFi();
    }

    // Maintain MQTT connection
    if (!mqtt.connected()) {
        unsigned long now = millis();
        if (now - lastMqttReconnect > MQTT_RECONNECT_INTERVAL) {
            lastMqttReconnect = now;
            connectMQTT();
        }
    }
    mqtt.loop();

    // Periodic BLE scan if not all beds found
    if (!allBedsFound) {
        unsigned long now = millis();
        if (now - lastBleScan > (BLE_SCAN_DURATION * 1000 + 5000)) {
            startBleScan();
        }
    }

    delay(10);
}
