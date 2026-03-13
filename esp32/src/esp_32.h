#pragma once

#include <Arduino.h>
#include <algorithm>
#include <cstdint>
#include <map>
#include <string>
#include <variant>
#include <vector>

enum class PinMode {
  Digital_Input,
  Digital_Output,
  PWM,
  Analog_Input,
  Analog_Output,
  Touch_Sensor,
  None
};

inline const char* pinModeToString(PinMode mode) {
  switch (mode) {
    case PinMode::Digital_Input:
      return "Digital_Input";
    case PinMode::Digital_Output:
      return "Digital_Output";
    case PinMode::PWM:
      return "PWM";
    case PinMode::Analog_Input:
      return "Analog_Input";
    case PinMode::Analog_Output:
      return "Analog_Output";
    case PinMode::Touch_Sensor:
      return "Touch_Sensor";
    case PinMode::None:
    default:
      return "None";
  }
}

inline PinMode pinModeFromString(const std::string& modeStr) {
  if (modeStr == "Digital_Input")
    return PinMode::Digital_Input;
  if (modeStr == "Digital_Output")
    return PinMode::Digital_Output;
  if (modeStr == "PWM")
    return PinMode::PWM;
  if (modeStr == "Analog_Input")
    return PinMode::Analog_Input;
  if (modeStr == "Analog_Output")
    return PinMode::Analog_Output;
  if (modeStr == "Touch_Sensor")
    return PinMode::Touch_Sensor;
  return PinMode::None;
}

enum class GPIOActorRole {
  Light,    // Digital
  Pump,     // Digital
  Relay,    // Digital
  Buzzer,   // Digital
  Heater,   // Digital
  Fan,      // PWM
  LedStrip, // PWM
  Valve,    // PWM
};

enum class GPIOSensorRole {
  Temperature,  // Analog
  Humidity,     // Analog
  LightSensor,  // Analog
  SoilMoisture, // Analog
  WaterLevel,   // Analog
  Motion,       // Digital
  DoorContact,  // Digital
  Switch,       // Digital
  Touch,        // Touch
  None          // Für abgeschaltete Pins
};

using GPIORole = std::variant<GPIOActorRole, GPIOSensorRole>;
using GPIORoles = std::vector<GPIORole>;

inline const char* gpioRoleToString(const GPIORole& role) {
  if (std::holds_alternative<GPIOActorRole>(role)) {
    switch (std::get<GPIOActorRole>(role)) {
      case GPIOActorRole::Light:
        return "Light";
      case GPIOActorRole::Pump:
        return "Pump";
      case GPIOActorRole::Relay:
        return "Relay";
      case GPIOActorRole::Buzzer:
        return "Buzzer";
      case GPIOActorRole::Heater:
        return "Heater";
      case GPIOActorRole::Fan:
        return "Fan";
      case GPIOActorRole::LedStrip:
        return "LedStrip";
      case GPIOActorRole::Valve:
        return "Valve";
    }
  } else if (std::holds_alternative<GPIOSensorRole>(role)) {
    switch (std::get<GPIOSensorRole>(role)) {
      case GPIOSensorRole::Temperature:
        return "Temperature";
      case GPIOSensorRole::Humidity:
        return "Humidity";
      case GPIOSensorRole::LightSensor:
        return "LightSensor";
      case GPIOSensorRole::SoilMoisture:
        return "SoilMoisture";
      case GPIOSensorRole::WaterLevel:
        return "WaterLevel";
      case GPIOSensorRole::Motion:
        return "Motion";
      case GPIOSensorRole::DoorContact:
        return "DoorContact";
      case GPIOSensorRole::Switch:
        return "Switch";
      case GPIOSensorRole::Touch:
        return "Touch";
      case GPIOSensorRole::None:
        return "None";
    }
  }
  return "Unknown";
}

inline GPIORole gpioRoleFromString(const std::string& roleStr) {
  if (roleStr == "Light")
    return GPIOActorRole::Light;
  if (roleStr == "Pump")
    return GPIOActorRole::Pump;
  if (roleStr == "Relay")
    return GPIOActorRole::Relay;
  if (roleStr == "Buzzer")
    return GPIOActorRole::Buzzer;
  if (roleStr == "Heater")
    return GPIOActorRole::Heater;
  if (roleStr == "Fan")
    return GPIOActorRole::Fan;
  if (roleStr == "LedStrip")
    return GPIOActorRole::LedStrip;
  if (roleStr == "Valve")
    return GPIOActorRole::Valve;

  if (roleStr == "Temperature")
    return GPIOSensorRole::Temperature;
  if (roleStr == "Humidity")
    return GPIOSensorRole::Humidity;
  if (roleStr == "LightSensor")
    return GPIOSensorRole::LightSensor;
  if (roleStr == "SoilMoisture")
    return GPIOSensorRole::SoilMoisture;
  if (roleStr == "WaterLevel")
    return GPIOSensorRole::WaterLevel;
  if (roleStr == "Motion")
    return GPIOSensorRole::Motion;
  if (roleStr == "DoorContact")
    return GPIOSensorRole::DoorContact;
  if (roleStr == "Switch")
    return GPIOSensorRole::Switch;
  if (roleStr == "Touch")
    return GPIOSensorRole::Touch;

  return GPIOSensorRole::None;
}

enum class DigitalOutputState { LOW_STATE, HIGH_STATE };

using PinValue = std::variant<bool, // Digital_Input/Output: true/false
                              int,
                              float,
                              DigitalOutputState,
                              std::monostate // Für Modi, die keinen Wert haben (z. B. None)
                              >;

struct Pin {
  std::string id;
  std::string pinNumber; // Physische Pin-Nummer auf dem Board
  std::string label;
  std::vector<PinMode> modes;
  GPIORoles roles;
  std::vector<std::string> specialFunctions; // optional, kann leer sein
  PinMode currentMode;
  GPIORole currentRole;
  PinValue value;

  // Typsichere Setter
  bool setValue(bool val) {
    if (currentMode == PinMode::Digital_Input) {
      value = val;
      return true;
    }
    return false;
  }

  bool setValue(DigitalOutputState val) {
    if (currentMode == PinMode::Digital_Output) {
      value = val;
      digitalWrite(std::stoi(pinNumber), val == DigitalOutputState::HIGH_STATE ? HIGH : LOW);
      return true;
    }
    return false;
  }

  bool setValue(int val) {
    if (currentMode == PinMode::PWM || currentMode == PinMode::Analog_Input) {
      value = val;
      return true;
    }
    return false;
  }

  bool setValue(float val) {
    if (currentMode == PinMode::Analog_Output) {
      value = val;
      return true;
    }
    return false;
  }

  bool setMode(PinMode mode) {
    const auto it = std::find(modes.begin(), modes.end(), mode);
    if (it == modes.end()) {
      Serial.println(
          ("Pin: " + pinNumber + " Modus " + pinModeToString(mode) + " nicht unterstützt.")
              .c_str());
      return false;
    }

    currentMode = mode;
    if (mode == PinMode::None) {
      pinMode(std::stoi(pinNumber),
              INPUT_PULLUP); // Setze Pin auf Eingang, um Stromverbrauch zu minimieren
      value = std::monostate{};
    } else if (mode == PinMode::Digital_Input) {
      value = false; // Standardwert für digitale Eingänge
    } else if (mode == PinMode::Digital_Output) {
      value = DigitalOutputState::LOW_STATE;   // Standardwert für digitale Ausgänge
      digitalWrite(std::stoi(pinNumber), LOW); // Setze physischen Pin auf LOW
    } else if (mode == PinMode::PWM || mode == PinMode::Analog_Input) {
      value = 0; // Standardwert für analoge Pins
    } else if (mode == PinMode::Analog_Output) {
      value = 0.0f; // Standardwert für analoge Ausgänge
    }
    return true;
  }

  bool setLabel(const std::string& newLabel) {
    if (label == newLabel) {
      Serial.println(("Pin: " + pinNumber + " Label unveraendert, kein Update noetig.").c_str());
      return false;
    }
    label = newLabel;
    return true;
  }

  bool setRole(const GPIORole& newRole) {
    if (currentRole == newRole) {
      Serial.println(("Pin: " + pinNumber + " Rolle unveraendert, kein Update noetig.").c_str());
      return false;
    }
    PinMode targetMode = PinMode::None;

    if (std::holds_alternative<GPIOActorRole>(newRole)) {
      switch (std::get<GPIOActorRole>(newRole)) {
        case GPIOActorRole::Light:
        case GPIOActorRole::Pump:
        case GPIOActorRole::Relay:
        case GPIOActorRole::Buzzer:
        case GPIOActorRole::Heater:
          targetMode = PinMode::Digital_Output;
          break;
        case GPIOActorRole::Fan:
        case GPIOActorRole::LedStrip:
        case GPIOActorRole::Valve:
          targetMode = PinMode::PWM;
          break;
      }
    } else if (std::holds_alternative<GPIOSensorRole>(newRole)) {
      switch (std::get<GPIOSensorRole>(newRole)) {
        case GPIOSensorRole::Temperature:
        case GPIOSensorRole::Humidity:
        case GPIOSensorRole::LightSensor:
        case GPIOSensorRole::SoilMoisture:
        case GPIOSensorRole::WaterLevel:
          targetMode = PinMode::Analog_Input;
          break;
        case GPIOSensorRole::Motion:
        case GPIOSensorRole::DoorContact:
        case GPIOSensorRole::Switch:
          targetMode = PinMode::Digital_Input;
          break;
        case GPIOSensorRole::Touch:
          targetMode = PinMode::Touch_Sensor;
          break;
        case GPIOSensorRole::None:
          targetMode = PinMode::None;
          break;
      }
    } else {
      Serial.println(("Pin: " + pinNumber + " Ungueltige Rolle: " + gpioRoleToString(newRole) +
                      ". Rolle muss entweder GPIOActorRole oder GPIOSensorRole sein.")
                         .c_str());
      return false;
    }

    if (!setMode(targetMode)) {
      return false;
    }

    currentRole = newRole;
    return true;
  }
};

struct Device {
  std::string type;
  std::string deviceName;
  std::uint32_t wifiScanInterval;
  std::map<int, Pin> pins;

  bool setDeviceName(const std::string& newName) {
    if (deviceName == newName) {
      return false;
    }
    deviceName = newName;
    return true;
  }

  bool setWifiScanInterval(std::uint32_t newInterval) {
    if (wifiScanInterval == newInterval) {
      return false;
    }
    wifiScanInterval = newInterval;
    return true;
  }

  bool setDevicePinsToStandard() {
    for (auto& [pinNum, pin] : pins) {
      pin.setMode(PinMode::None);
    }
    return true;
  }
};

extern const Device ESP32_Custom;
extern const Device ESP32_Dev_Kit_C_V4;