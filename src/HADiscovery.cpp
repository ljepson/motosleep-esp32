#include "HADiscovery.h"

HADiscovery::HADiscovery(PubSubClient& mqtt) : _mqtt(mqtt) {
}

String HADiscovery::getDiscoveryTopic(const char* component, const BedConfig& bed, const char* entityId) {
    // Format: homeassistant/{component}/{device_id}_{entity_id}/config
    String topic = HA_DISCOVERY_PREFIX;
    topic += "/";
    topic += component;
    topic += "/";
    topic += DEVICE_NAME;
    topic += "_";
    topic += bed.id;
    topic += "_";
    topic += entityId;
    topic += "/config";
    return topic;
}

String HADiscovery::getCommandTopic(const BedConfig& bed, const char* entityId) {
    // Format: motosleep/{bed_id}/{entity_id}/set
    String topic = "motosleep/";
    topic += bed.id;
    topic += "/";
    topic += entityId;
    topic += "/set";
    return topic;
}

String HADiscovery::getStateTopic(const BedConfig& bed) {
    // Format: motosleep/{bed_id}/state
    String topic = "motosleep/";
    topic += bed.id;
    topic += "/state";
    return topic;
}

void HADiscovery::addDeviceInfo(JsonObject& device, const BedConfig& bed) {
    device["identifiers"][0] = String(DEVICE_NAME) + "_" + bed.id;
    device["name"] = bed.friendlyName;
    device["manufacturer"] = "MotoSleep";
    device["model"] = "Adjustable Bed";
    device["via_device"] = DEVICE_NAME;
}

void HADiscovery::addControllerDeviceInfo(JsonObject& device) {
    device["identifiers"][0] = DEVICE_NAME;
    device["name"] = DEVICE_FRIENDLY_NAME;
    device["manufacturer"] = "DIY";
    device["model"] = "ESP32 MotoSleep Controller";
    device["sw_version"] = "1.0.0";
}

void HADiscovery::publishButton(const BedConfig& bed, const MotoSleep::Command& cmd) {
    JsonDocument doc;

    // Unique ID
    String uniqueId = String(DEVICE_NAME) + "_" + bed.id + "_" + cmd.name;
    doc["unique_id"] = uniqueId;

    // Name
    doc["name"] = cmd.friendlyName;

    // Command topic
    doc["command_topic"] = getCommandTopic(bed, cmd.name);

    // Payload
    doc["payload_press"] = "PRESS";

    // Icon
    if (cmd.icon) {
        doc["icon"] = cmd.icon;
    }

    // Device info
    JsonObject device = doc["device"].to<JsonObject>();
    addDeviceInfo(device, bed);

    // Availability
    doc["availability_topic"] = "motosleep/status";
    doc["payload_available"] = "online";
    doc["payload_not_available"] = "offline";

    // Serialize and publish
    String payload;
    serializeJson(doc, payload);

    String topic = getDiscoveryTopic("button", bed, cmd.name);
    _mqtt.publish(topic.c_str(), payload.c_str(), true);

    Serial.printf("[HA] Published button: %s\n", cmd.name);
}

void HADiscovery::publishBedDiscovery(const BedConfig& bed) {
    Serial.printf("[HA] Publishing discovery for bed: %s\n", bed.friendlyName);

    // Publish motor control buttons
    for (size_t i = 0; i < MotoSleep::MOTOR_COMMAND_COUNT; i++) {
        publishButton(bed, MotoSleep::MOTOR_COMMANDS[i]);
    }

    // Publish preset buttons
    for (size_t i = 0; i < MotoSleep::PRESET_COMMAND_COUNT; i++) {
        publishButton(bed, MotoSleep::PRESET_COMMANDS[i]);
    }

    // Publish program buttons
    for (size_t i = 0; i < MotoSleep::PROGRAM_COMMAND_COUNT; i++) {
        publishButton(bed, MotoSleep::PROGRAM_COMMANDS[i]);
    }

    // Publish massage buttons
    for (size_t i = 0; i < MotoSleep::MASSAGE_COMMAND_COUNT; i++) {
        publishButton(bed, MotoSleep::MASSAGE_COMMANDS[i]);
    }

    // Publish light buttons
    for (size_t i = 0; i < MotoSleep::LIGHT_COMMAND_COUNT; i++) {
        publishButton(bed, MotoSleep::LIGHT_COMMANDS[i]);
    }

    Serial.printf("[HA] Discovery complete for bed: %s\n", bed.friendlyName);
}

void HADiscovery::removeBedDiscovery(const BedConfig& bed) {
    // Publish empty payload to remove entities
    auto removeEntity = [this, &bed](const MotoSleep::Command& cmd) {
        String topic = getDiscoveryTopic("button", bed, cmd.name);
        _mqtt.publish(topic.c_str(), "", true);
    };

    for (size_t i = 0; i < MotoSleep::MOTOR_COMMAND_COUNT; i++) {
        removeEntity(MotoSleep::MOTOR_COMMANDS[i]);
    }
    for (size_t i = 0; i < MotoSleep::PRESET_COMMAND_COUNT; i++) {
        removeEntity(MotoSleep::PRESET_COMMANDS[i]);
    }
    for (size_t i = 0; i < MotoSleep::PROGRAM_COMMAND_COUNT; i++) {
        removeEntity(MotoSleep::PROGRAM_COMMANDS[i]);
    }
    for (size_t i = 0; i < MotoSleep::MASSAGE_COMMAND_COUNT; i++) {
        removeEntity(MotoSleep::MASSAGE_COMMANDS[i]);
    }
    for (size_t i = 0; i < MotoSleep::LIGHT_COMMAND_COUNT; i++) {
        removeEntity(MotoSleep::LIGHT_COMMANDS[i]);
    }
}

void HADiscovery::publishControllerDiscovery() {
    // Publish a sensor for the controller status
    JsonDocument doc;

    String uniqueId = String(DEVICE_NAME) + "_status";
    doc["unique_id"] = uniqueId;
    doc["name"] = "Controller Status";
    doc["state_topic"] = "motosleep/status";
    doc["icon"] = "mdi:chip";

    JsonObject device = doc["device"].to<JsonObject>();
    addControllerDeviceInfo(device);

    String payload;
    serializeJson(doc, payload);

    String topic = String(HA_DISCOVERY_PREFIX) + "/sensor/" + DEVICE_NAME + "_status/config";
    _mqtt.publish(topic.c_str(), payload.c_str(), true);

    Serial.println("[HA] Published controller status sensor");
}
