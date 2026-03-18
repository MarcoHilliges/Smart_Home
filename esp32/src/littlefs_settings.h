#ifndef LITTLEFS_SETTINGS_H
#define LITTLEFS_SETTINGS_H

#include <ArduinoJson.h>
#include <LittleFS.h>

// ----------------------------------------
// LittleFS Settings Verwaltung
// Speichert und lädt Gerätekonfigurationen aus dem Flash-Speicher
// ----------------------------------------

#define SETTINGS_FILE "/settings.json"
#define FORMAT_LITTLEFS_IF_FAILED true

extern Device device; // Aktuelles Device mit Pin-Definitionen

// ----------------------------------------
// Funktion: initLittleFS
// Initialisiert das LittleFS-Dateisystem
// ----------------------------------------
bool initLittleFS() {
  Serial.println("LittleFS wird initialisiert...");

  if (!LittleFS.begin(FORMAT_LITTLEFS_IF_FAILED)) {
    Serial.println("LittleFS MOUNT FAILED!");
    return false;
  }

  Serial.println("LittleFS erfolgreich gemountet!");

  // Debug: Zeige verfügbaren Speicher
  size_t totalBytes = LittleFS.totalBytes();
  size_t usedBytes = LittleFS.usedBytes();
  Serial.printf("LittleFS Speicher - Gesamt: %u Bytes, Verwendet: %u Bytes, Frei: %u Bytes\n",
                totalBytes,
                usedBytes,
                totalBytes - usedBytes);

  return true;
}

// ----------------------------------------
// Funktion: saveSettings
// Speichert die aktuellen Einstellungen in die LittleFS-Datei
// Gibt true zurück, wenn erfolgreich, false wenn Fehler auftrat
// ----------------------------------------
bool saveSettings() {
  Serial.println("Speichere Settings auf LittleFS...");

  // Erstelle ein JSON-Dokument
  DynamicJsonDocument doc(512);

  // Füge die aktuellen Einstellungen hinzu
  doc["wifiScanInterval"] = device.wifiScanInterval;
  doc["deviceName"] = device.deviceName;

  JsonObject gpioObj = doc.createNestedObject("gpioConfigs");
  for (const auto& [pinNum, pin] : device.pins) {
    JsonObject p = gpioObj.createNestedObject(String(pinNum));
    p["id"] = pin.id;
    p["pinNumber"] = pin.pinNumber;
    p["label"] = pin.label;
    p["currentRole"] = gpioRoleToString(pin.currentRole);

    if (std::holds_alternative<bool>(pin.value)) {
      p["value"] = std::get<bool>(pin.value);
    } else if (std::holds_alternative<int>(pin.value)) {
      p["value"] = std::get<int>(pin.value);
    } else if (std::holds_alternative<float>(pin.value)) {
      p["value"] = std::get<float>(pin.value);
    } else if (std::holds_alternative<DigitalOutputState>(pin.value)) {
      p["value"] =
          std::get<DigitalOutputState>(pin.value) == DigitalOutputState::HIGH_STATE ? 1 : 0;
    } else {
      p["value"] = nullptr;
    }
  }

  // Öffne die Datei zum Schreiben (überschreibe, falls sie existiert)
  File settingsFile = LittleFS.open(SETTINGS_FILE, "w");
  if (!settingsFile) {
    Serial.println("Fehler beim Öffnen der Settings-Datei zum Schreiben!");
    return false;
  }

  // Serialisiere und schreibe die JSON in die Datei
  size_t bytesWritten = serializeJson(doc, settingsFile);
  settingsFile.close(); // Datei schließen

  if (bytesWritten == 0) {
    Serial.println("Fehler beim Schreiben der Settings!");
    return false;
  }

  Serial.print("Settings erfolgreich gespeichert (");
  Serial.print(bytesWritten);
  Serial.println(" Bytes)");

  return true;
}

// ----------------------------------------
// Funktion: loadSettings
// Lädt die Einstellungen aus der LittleFS-Datei
// Gibt true zurück, wenn erfolgreich, false wenn Fehler auftrat
// ----------------------------------------
bool loadSettings() {
  Serial.println("Versuche Settings von LittleFS zu laden...");

  // Überprüfe, ob die Datei existiert
  if (!LittleFS.exists(SETTINGS_FILE)) {
    Serial.println("Settings-Datei nicht gefunden. Verwende Standard-Einstellungen.");
    // Speichere die Standard-Einstellungen
    return saveSettings();
  }

  // Öffne die Datei zum Lesen
  File settingsFile = LittleFS.open(SETTINGS_FILE, "r");
  if (!settingsFile) {
    Serial.println("Fehler beim Öffnen der Settings-Datei!");
    return false;
  }

  // Erstelle ein JSON-Dokument (512 Bytes reichen aus)
  DynamicJsonDocument doc(512);

  // Versuche, die JSON zu parsen
  DeserializationError error = deserializeJson(doc, settingsFile);
  settingsFile.close(); // Datei schließen

  if (error) {
    Serial.print("Fehler beim Parsen der Settings-JSON: ");
    Serial.println(error.f_str());
    return false;
  }

  // Extrahiere die Einstellungen aus der JSON
  if (doc.containsKey("wifiScanInterval")) {
    device.wifiScanInterval = doc["wifiScanInterval"].as<uint32_t>();
    Serial.print("wifiScanInterval geladen: ");
    Serial.println(device.wifiScanInterval);
  }

  if (doc.containsKey("deviceName")) {
    device.deviceName = doc["deviceName"].as<const char*>();
    Serial.print("deviceName geladen: ");
    Serial.println(device.deviceName.c_str());
  }

  // Lade GPIO-Metadaten falls vorhanden
  if (doc.containsKey("gpioConfigs") && doc["gpioConfigs"].is<JsonObject>()) {
    JsonObject ga = doc["gpioConfigs"].as<JsonObject>();
    for (JsonPair kvp : ga) {
      int pinNum = atoi(kvp.key().c_str());
      auto it = device.pins.find(pinNum);
      if (it != device.pins.end()) {
        JsonObject g = kvp.value().as<JsonObject>();
        Pin& pin = it->second;

        if (g.containsKey("id"))
          pin.id = g["id"].as<const char*>();
        if (g.containsKey("pinNumber"))
          pin.pinNumber = g["pinNumber"].as<const char*>();
        if (g.containsKey("label"))
          pin.label = g["label"].as<const char*>();

        if (g.containsKey("currentRole") && g["currentRole"].is<const char*>()) {
          pin.setRole(gpioRoleFromString(g["currentRole"].as<const char*>()));
        }

        if (g.containsKey("value")) {
          switch (pin.currentMode) {
            case PinMode::Digital_Output:
              pin.setValue(g["value"].as<int>() > 0 ? DigitalOutputState::HIGH_STATE
                                                    : DigitalOutputState::LOW_STATE);
              break;
            case PinMode::Digital_Input:
              pin.setValue(g["value"].as<bool>());
              break;
            case PinMode::Analog_Output:
              pin.setValue(g["value"].as<float>());
              break;
            case PinMode::PWM:
            case PinMode::Analog_Input:
              pin.setValue(g["value"].as<int>());
              break;
            case PinMode::Touch_Sensor:
            case PinMode::None:
            default:
              break;
          }
        }
      }
    }

    Serial.println("GPIO-Metadaten geladen aus Settings.");
  }

  Serial.println("Settings erfolgreich geladen!");
  return true;
}

// ----------------------------------------
// Funktion: deleteSettings
// Löscht die Settings-Datei aus dem LittleFS
// ----------------------------------------
bool deleteSettings() {
  Serial.println("Lösche Settings-Datei...");

  if (LittleFS.remove(SETTINGS_FILE)) {
    Serial.println("Settings-Datei erfolgreich gelöscht!");
    return true;
  } else {
    Serial.println("Fehler beim Löschen der Settings-Datei!");
    return false;
  }
}

// ----------------------------------------
// Funktion: printSettings
// Debug-Funktion: Zeigt die aktuellen Einstellungen auf der Konsole
// ----------------------------------------
void printSettings() {
  Serial.println("\n========== Aktuelle Einstellungen ==========");
  Serial.print("WiFi Scan Intervall: ");
  Serial.print(device.wifiScanInterval);
  Serial.println(" ms");
  Serial.print("Gerätename: ");
  Serial.println(device.deviceName.c_str());
  // GPIO Metadata ausgeben (falls definiert)
  Serial.println("GPIO Metadaten:");
  for (const auto& [pinNum, pin] : device.pins) {
    Serial.print("  Pin ");
    Serial.print(pinNum);
    Serial.print(" - Mode: ");
    Serial.print(static_cast<int>(pin.currentMode));
    Serial.print(" - Label: ");
    Serial.println(pin.label.c_str());
  }

  Serial.println("===========================================\n");
}

#endif // LITTLEFS_SETTINGS_H
