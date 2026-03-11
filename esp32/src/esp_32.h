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
  std::string label;     // Optional, kann leer sein
  std::vector<PinMode> modes;
  std::vector<std::string> specialFunctions; // optional, kann leer sein
  PinMode currentMode;
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

  PinMode convertStringToPinMode(const std::string& modeStr) {
    return pinModeFromString(modeStr);
  }

  bool setMode(PinMode mode) {
    const auto it = std::find(modes.begin(), modes.end(), mode);
    if (it == modes.end()) {
      return false; // Modus nicht unterstützt
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
      return false;
    }
    label = newLabel;
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