#ifndef LITTLEFS_SETTINGS_H
#define LITTLEFS_SETTINGS_H

#include <LittleFS.h>
#include <ArduinoJson.h>

// ----------------------------------------
// LittleFS Settings Verwaltung
// Speichert und lädt Gerätekonfigurationen aus dem Flash-Speicher
// ----------------------------------------

#define SETTINGS_FILE "/settings.json"
#define FORMAT_LITTLEFS_IF_FAILED true

// Struktur für die Geräteeinstellungen
struct DeviceSettings {
  long wifiScanInterval = 60000;  // Standard: 60 Sekunden
  String deviceName = "ESP32-Dashboard";
};

// Globale Instanz für die aktuellen Einstellungen
DeviceSettings deviceSettings;

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
    totalBytes, usedBytes, totalBytes - usedBytes);
  
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
  doc["wifiScanInterval"] = deviceSettings.wifiScanInterval;
  doc["deviceName"] = deviceSettings.deviceName;

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
    deviceSettings.wifiScanInterval = doc["wifiScanInterval"].as<long>();
    Serial.print("wifiScanInterval geladen: ");
    Serial.println(deviceSettings.wifiScanInterval);
  }

  if (doc.containsKey("deviceName")) {
    deviceSettings.deviceName = doc["deviceName"].as<String>();
    Serial.print("deviceName geladen: ");
    Serial.println(deviceSettings.deviceName);
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
  Serial.print(deviceSettings.wifiScanInterval);
  Serial.println(" ms");
  Serial.print("Gerätename: ");
  Serial.println(deviceSettings.deviceName);
  Serial.println("===========================================\n");
}

#endif // LITTLEFS_SETTINGS_H
