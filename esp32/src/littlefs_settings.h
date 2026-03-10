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

// Struktur für GPIO-Metadaten (Label / Mode)
// struct GPIOConfig {
//   int pinNumber = -1;
//   String mode = "none"; // "lamp" | "pump" | "none"
//   String label = "";
// };

// Externe Referenzen: werden in main.cpp definiert
// extern const int NUM_PINS;
// extern GPIOConfig gpioConfigs[];

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
    p["currentMode"] = pin.currentMode;
    p["value"] = pin.value;
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
      if (device.pins.find(pinNum) != device.pins.end()) {
        JsonObject g = kvp.value().as<JsonObject>();
        if (g.containsKey("id"))
          device.pins[pinNum].id = g["id"].as<int>();
        if (g.containsKey("pinNumber"))
          device.pins[pinNum].pinNumber = g["pinNumber"].as<int>();
        if (g.containsKey("label"))
          device.pins[pinNum].label = g["label"].as<const char*>();
        if (g.containsKey("currentMode"))
          device.pins[pinNum].setMode(g["currentMode"] | PinMode::None);
        if (g.containsKey("value")) {
          device.pins[pinNum].value = g["value"].as<int>();
          if (device.pins[pinNum].currentMode == PinMode::Digital_Output) {
            device.pins[pinNum].setValue(g["value"].as<int>() ==
                                                 static_cast<int>(DigitalOutputState::HIGH_STATE)
                                             ? DigitalOutputState::HIGH_STATE
                                             : DigitalOutputState::LOW_STATE);
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
