#include <algorithm>
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

using PinValue = std::variant<bool, // Digital_Input/Output: true/false
                              int,
                              float,
                              std::monostate // Für Modi, die keinen Wert haben (z. B. None)
                              >;

struct Pin {
  std::string name;
  std::vector<PinMode> modes;
  std::vector<std::string> specialFunctions; // optional, kann leer sein
  PinMode currentMode;
  PinValue value;

  // Typsichere Setter
  bool setValue(bool val) {
    if (currentMode == PinMode::Digital_Input || currentMode == PinMode::Digital_Output) {
      value = val;
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
      return false; // Modus nicht unterstützt
    }

    currentMode = mode;
    if (mode == PinMode::None) {
      value = std::monostate{};
    } else if (mode == PinMode::Digital_Input || mode == PinMode::Digital_Output) {
      value = false; // Standardwert für digitale Pins
    } else if (mode == PinMode::PWM || mode == PinMode::Analog_Input) {
      value = 0; // Standardwert für analoge Pins
    } else if (mode == PinMode::Analog_Output) {
      value = 0.0f; // Standardwert für analoge Ausgänge
    }
    return true;
  }
};

struct Device {
  std::string type;
  std::map<int, Pin> pins; // Schlüssel: Pin-Nummer (z. B. 0,1,2,...)
};