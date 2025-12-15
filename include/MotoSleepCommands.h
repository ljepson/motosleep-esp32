#ifndef MOTOSLEEP_COMMANDS_H
#define MOTOSLEEP_COMMANDS_H

#include <Arduino.h>

// =============================================================================
// MotoSleep BLE Service/Characteristic UUIDs
// =============================================================================
#define MOTOSLEEP_SERVICE_UUID        "0000ffe0-0000-1000-8000-00805f9b34fb"
#define MOTOSLEEP_CHARACTERISTIC_UUID "0000ffe1-0000-1000-8000-00805f9b34fb"

// =============================================================================
// MotoSleep Command Protocol
// All commands are 2 bytes: [0x24, command_char]
// 0x24 = '$' prefix
// =============================================================================

namespace MotoSleep {

// Command prefix (all commands start with this)
constexpr uint8_t CMD_PREFIX = 0x24;  // '$'

// Motor control commands
namespace Motor {
    constexpr char HEAD_UP    = 'K';
    constexpr char HEAD_DOWN  = 'L';
    constexpr char FEET_UP    = 'M';
    constexpr char FEET_DOWN  = 'N';
    constexpr char NECK_UP    = 'P';  // Also used for Lumbar on some models
    constexpr char NECK_DOWN  = 'Q';
    constexpr char LUMBAR_UP  = 'P';  // Same as NECK_UP
    constexpr char LUMBAR_DOWN = 'Q'; // Same as NECK_DOWN
    // Secondary lumbar (some models)
    constexpr char LUMBAR2_UP   = 'p';
    constexpr char LUMBAR2_DOWN = 'q';
    constexpr char TILT_UP    = 'P';  // Model dependent
    constexpr char TILT_DOWN  = 'Q';
    constexpr char ALL_UP     = 'p';  // Some models
    constexpr char ALL_DOWN   = 'q';
}

// Preset commands
namespace Preset {
    constexpr char HOME       = 'O';
    constexpr char MEMORY_1   = 'U';
    constexpr char MEMORY_2   = 'V';
    constexpr char ANTI_SNORE = 'R';
    constexpr char TV         = 'S';
    constexpr char ZERO_G     = 'T';
}

// Preset programming commands (save current position)
namespace Program {
    constexpr char MEMORY_1   = 'Z';
    constexpr char MEMORY_2   = 'a';
    constexpr char ANTI_SNORE = 'W';
    constexpr char TV         = 'X';
    constexpr char ZERO_G     = 'Y';
}

// Massage commands
namespace Massage {
    constexpr char HEAD_STEP  = 'C';  // Cycle through head massage intensities
    constexpr char FOOT_STEP  = 'B';  // Cycle through foot massage intensities
    constexpr char STOP       = 'D';  // Stop all motors and massage
    constexpr char HEAD_OFF   = 'J';
    constexpr char FOOT_OFF   = 'I';
    // Intensity control (some models)
    constexpr char HEAD_UP    = 'G';
    constexpr char HEAD_DOWN  = 'H';
    constexpr char FOOT_UP    = 'E';
    constexpr char FOOT_DOWN  = 'F';
}

// Light commands
namespace Light {
    constexpr char TOGGLE     = 'A';  // Toggle under-bed lights
}

// Command structure
struct Command {
    const char* name;           // Command name for MQTT/HA
    const char* friendlyName;   // Display name
    char cmdChar;               // Command character
    const char* category;       // Category for grouping (motor, preset, massage, light)
    const char* icon;           // Material Design Icon for HA
};

// Motor commands (these are "hold" commands - send continuously while button held)
const Command MOTOR_COMMANDS[] = {
    {"head_up",    "Head Up",    Motor::HEAD_UP,    "motor", "mdi:arrow-up-bold"},
    {"head_down",  "Head Down",  Motor::HEAD_DOWN,  "motor", "mdi:arrow-down-bold"},
    {"feet_up",    "Feet Up",    Motor::FEET_UP,    "motor", "mdi:arrow-up-bold"},
    {"feet_down",  "Feet Down",  Motor::FEET_DOWN,  "motor", "mdi:arrow-down-bold"},
    {"neck_up",    "Neck Up",    Motor::NECK_UP,    "motor", "mdi:arrow-up-bold"},
    {"neck_down",  "Neck Down",  Motor::NECK_DOWN,  "motor", "mdi:arrow-down-bold"},
};
constexpr size_t MOTOR_COMMAND_COUNT = sizeof(MOTOR_COMMANDS) / sizeof(MOTOR_COMMANDS[0]);

// Preset commands (single press)
const Command PRESET_COMMANDS[] = {
    {"preset_home",       "Flat/Home",    Preset::HOME,       "preset", "mdi:bed"},
    {"preset_memory_1",   "Memory 1",     Preset::MEMORY_1,   "preset", "mdi:numeric-1-box"},
    {"preset_memory_2",   "Memory 2",     Preset::MEMORY_2,   "preset", "mdi:numeric-2-box"},
    {"preset_anti_snore", "Anti-Snore",   Preset::ANTI_SNORE, "preset", "mdi:sleep"},
    {"preset_tv",         "TV",           Preset::TV,         "preset", "mdi:television"},
    {"preset_zero_g",     "Zero Gravity", Preset::ZERO_G,     "preset", "mdi:rocket-launch"},
};
constexpr size_t PRESET_COMMAND_COUNT = sizeof(PRESET_COMMANDS) / sizeof(PRESET_COMMANDS[0]);

// Program commands (save current position to preset)
const Command PROGRAM_COMMANDS[] = {
    {"program_memory_1",   "Save Memory 1",     Program::MEMORY_1,   "config", "mdi:content-save"},
    {"program_memory_2",   "Save Memory 2",     Program::MEMORY_2,   "config", "mdi:content-save"},
    {"program_anti_snore", "Save Anti-Snore",   Program::ANTI_SNORE, "config", "mdi:content-save"},
    {"program_tv",         "Save TV",           Program::TV,         "config", "mdi:content-save"},
    {"program_zero_g",     "Save Zero Gravity", Program::ZERO_G,     "config", "mdi:content-save"},
};
constexpr size_t PROGRAM_COMMAND_COUNT = sizeof(PROGRAM_COMMANDS) / sizeof(PROGRAM_COMMANDS[0]);

// Massage commands
const Command MASSAGE_COMMANDS[] = {
    {"massage_head_step", "Head Massage",   Massage::HEAD_STEP, "massage", "mdi:vibrate"},
    {"massage_foot_step", "Foot Massage",   Massage::FOOT_STEP, "massage", "mdi:vibrate"},
    {"massage_head_off",  "Head Massage Off", Massage::HEAD_OFF, "massage", "mdi:vibrate-off"},
    {"massage_foot_off",  "Foot Massage Off", Massage::FOOT_OFF, "massage", "mdi:vibrate-off"},
    {"massage_stop",      "Stop All",       Massage::STOP,      "massage", "mdi:stop"},
};
constexpr size_t MASSAGE_COMMAND_COUNT = sizeof(MASSAGE_COMMANDS) / sizeof(MASSAGE_COMMANDS[0]);

// Light commands
const Command LIGHT_COMMANDS[] = {
    {"light_toggle", "Under-Bed Lights", Light::TOGGLE, "light", "mdi:lightbulb"},
};
constexpr size_t LIGHT_COMMAND_COUNT = sizeof(LIGHT_COMMANDS) / sizeof(LIGHT_COMMANDS[0]);

// Helper to build a 2-byte command
inline void buildCommand(uint8_t* buffer, char cmdChar) {
    buffer[0] = CMD_PREFIX;
    buffer[1] = static_cast<uint8_t>(cmdChar);
}

} // namespace MotoSleep

#endif // MOTOSLEEP_COMMANDS_H
