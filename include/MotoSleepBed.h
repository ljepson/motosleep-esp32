#ifndef MOTOSLEEP_BED_H
#define MOTOSLEEP_BED_H

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEClient.h>
#include <BLEUtils.h>
#include "MotoSleepCommands.h"
#include "config.h"

class MotoSleepBed {
public:
    enum class State {
        DISCONNECTED,
        CONNECTING,
        CONNECTED,
        ERROR
    };

    MotoSleepBed(const BedConfig& config);
    ~MotoSleepBed();

    // Connection management
    bool connect();
    void disconnect();
    bool isConnected() const;
    State getState() const { return _state; }

    // Set the BLE address after scanning
    void setAddress(BLEAddress* address);
    bool hasAddress() const { return _address != nullptr; }

    // Send a command to the bed
    bool sendCommand(char cmdChar);
    bool sendCommand(const uint8_t* data, size_t len);

    // Getters
    const char* getBleName() const { return _config.bleName; }
    const char* getFriendlyName() const { return _config.friendlyName; }
    const char* getId() const { return _config.id; }

    // For reconnection timing
    unsigned long getLastConnectAttempt() const { return _lastConnectAttempt; }
    void setLastConnectAttempt(unsigned long time) { _lastConnectAttempt = time; }

private:
    BedConfig _config;
    BLEAddress* _address = nullptr;
    BLEClient* _client = nullptr;
    BLERemoteCharacteristic* _characteristic = nullptr;
    State _state = State::DISCONNECTED;
    unsigned long _lastConnectAttempt = 0;

    bool connectToServer();
    void cleanup();
};

// Callback class for BLE client connection events
class BedClientCallback : public BLEClientCallbacks {
public:
    BedClientCallback(MotoSleepBed* bed) : _bed(bed) {}

    void onConnect(BLEClient* client) override;
    void onDisconnect(BLEClient* client) override;

private:
    MotoSleepBed* _bed;
};

#endif // MOTOSLEEP_BED_H
