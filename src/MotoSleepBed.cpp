#include "MotoSleepBed.h"

static const char* TAG = "MotoSleepBed";

MotoSleepBed::MotoSleepBed(const BedConfig& config) : _config(config) {
}

MotoSleepBed::~MotoSleepBed() {
    cleanup();
    if (_address) {
        delete _address;
        _address = nullptr;
    }
}

void MotoSleepBed::setAddress(BLEAddress* address) {
    if (_address) {
        delete _address;
    }
    _address = new BLEAddress(*address);
}

bool MotoSleepBed::connect() {
    if (_state == State::CONNECTED && _client && _client->isConnected()) {
        return true;
    }

    if (!_address) {
        Serial.printf("[%s] No BLE address set, cannot connect\n", _config.friendlyName);
        return false;
    }

    _state = State::CONNECTING;
    _lastConnectAttempt = millis();

    Serial.printf("[%s] Connecting to %s...\n", _config.friendlyName, _address->toString().c_str());

    if (!connectToServer()) {
        _state = State::ERROR;
        return false;
    }

    _state = State::CONNECTED;
    Serial.printf("[%s] Connected!\n", _config.friendlyName);
    return true;
}

bool MotoSleepBed::connectToServer() {
    cleanup();

    _client = BLEDevice::createClient();
    _client->setClientCallbacks(new BedClientCallback(this));

    // Connect to the BLE server
    if (!_client->connect(*_address)) {
        Serial.printf("[%s] Failed to connect to BLE server\n", _config.friendlyName);
        cleanup();
        return false;
    }

    // Get the service
    BLERemoteService* service = _client->getService(BLEUUID(MOTOSLEEP_SERVICE_UUID));
    if (!service) {
        Serial.printf("[%s] Failed to find service UUID: %s\n", _config.friendlyName, MOTOSLEEP_SERVICE_UUID);
        cleanup();
        return false;
    }

    // Get the characteristic
    _characteristic = service->getCharacteristic(BLEUUID(MOTOSLEEP_CHARACTERISTIC_UUID));
    if (!_characteristic) {
        Serial.printf("[%s] Failed to find characteristic UUID: %s\n", _config.friendlyName, MOTOSLEEP_CHARACTERISTIC_UUID);
        cleanup();
        return false;
    }

    // Check if we can write to it
    if (!_characteristic->canWrite()) {
        Serial.printf("[%s] Characteristic is not writable\n", _config.friendlyName);
        cleanup();
        return false;
    }

    return true;
}

void MotoSleepBed::disconnect() {
    if (_client && _client->isConnected()) {
        Serial.printf("[%s] Disconnecting...\n", _config.friendlyName);
        _client->disconnect();
    }
    cleanup();
    _state = State::DISCONNECTED;
}

void MotoSleepBed::cleanup() {
    _characteristic = nullptr;
    if (_client) {
        delete _client;
        _client = nullptr;
    }
}

bool MotoSleepBed::isConnected() const {
    return _state == State::CONNECTED && _client && _client->isConnected();
}

bool MotoSleepBed::sendCommand(char cmdChar) {
    uint8_t cmd[2];
    MotoSleep::buildCommand(cmd, cmdChar);
    return sendCommand(cmd, 2);
}

bool MotoSleepBed::sendCommand(const uint8_t* data, size_t len) {
    // Connect if not already connected
    if (!isConnected()) {
        if (!connect()) {
            Serial.printf("[%s] Cannot send command - connection failed\n", _config.friendlyName);
            return false;
        }
    }

    if (!_characteristic) {
        Serial.printf("[%s] No characteristic available\n", _config.friendlyName);
        return false;
    }

    Serial.printf("[%s] Sending command: 0x%02X 0x%02X\n", _config.friendlyName, data[0], data[1]);

    // Write the command
    _characteristic->writeValue((uint8_t*)data, len, false);

    // If not staying connected, disconnect after sending
    #if !BLE_STAY_CONNECTED
    delay(100);  // Give the command time to be sent
    disconnect();
    #endif

    return true;
}

// BLE Client Callbacks
void BedClientCallback::onConnect(BLEClient* client) {
    Serial.printf("[BLE] Client connected\n");
}

void BedClientCallback::onDisconnect(BLEClient* client) {
    Serial.printf("[BLE] Client disconnected\n");
}
