#ifndef HA_DISCOVERY_H
#define HA_DISCOVERY_H

#include <Arduino.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "config.h"
#include "MotoSleepCommands.h"

class HADiscovery {
public:
    HADiscovery(PubSubClient& mqtt);

    // Publish discovery configs for a bed
    void publishBedDiscovery(const BedConfig& bed);

    // Remove discovery configs for a bed
    void removeBedDiscovery(const BedConfig& bed);

    // Publish controller device discovery (status, etc.)
    void publishControllerDiscovery();

private:
    PubSubClient& _mqtt;

    // Helper to publish a button entity
    void publishButton(const BedConfig& bed, const MotoSleep::Command& cmd);

    // Helper to publish device info
    void addDeviceInfo(JsonObject& device, const BedConfig& bed);
    void addControllerDeviceInfo(JsonObject& device);

    // Build topic strings
    String getDiscoveryTopic(const char* component, const BedConfig& bed, const char* entityId);
    String getCommandTopic(const BedConfig& bed, const char* entityId);
    String getStateTopic(const BedConfig& bed);
};

#endif // HA_DISCOVERY_H
