#include "esp_32.h"
#include "littlefs_settings.h" // LittleFS-Verwaltung für Geräteeinstellungen
#include "secrets.h"      // Enthält vertrauliche WLAN- und MQTT-Zugangsdaten. MUSS in .gitignore!
#include <ArduinoJson.h>  // Bibliothek für effizientes JSON-Parsing und -Generierung
#include <Esp.h>          // Für ESP-spezifische Funktionen wie ESP.getFreeHeap()
#include <PubSubClient.h> // Bibliothek für MQTT-Kommunikation
#include <WiFi.h>         // Bibliothek für WLAN-Funktionalität

// ----------------------------------------
// WLAN-Einstellungen
// Werden aus der secrets.h-Datei geladen, um sie vom Code zu trennen
// ----------------------------------------
const char* ssid = WIFI_SSID;         // WLAN-SSID
const char* password = WIFI_PASSWORD; // WLAN-Passwort

// ----------------------------------------
// MQTT-Einstellungen
// Werden ebenfalls aus der secrets.h-Datei geladen
// ----------------------------------------
const char* mqtt_broker = MQTT_BROKER_IP; // IP-Adresse des MQTT-Brokers (dein Docker-Host)
const int mqtt_port = MQTT_BROKER_PORT;   // MQTT-Port des Brokers (Standard 1883)
const char* mqtt_user = MQTT_USERNAME;    // MQTT-Benutzername
const char* mqtt_pass = MQTT_PASSWORD;    // MQTT-Passwort

// Globale Variablen für die Geräte-ID und MQTT-Topics.
// Diese werden dynamisch in setup() initialisiert, da deviceId von der MAC abhängt.
String deviceId;                 // Eindeutige Geräte-ID basierend auf der MAC-Adresse
String topic_status_get_all_sub; // Topic zum Abonnieren von Anfragen für den
                                 // Online-Status/Heartbeats aller Geräte
String topic_status_pub;         // Topic zum Veröffentlichen des Online-Status/Heartbeats
String topic_status_get_sub;     // Topic zum Abonnieren von Anfragen für den Status
String topic_wifi_scan_pub;      // Topic zum Veröffentlichen von WiFi-Scan-Ergebnissen
String topic_wifi_get_sub;       // Topic zum Abonnieren von Anfragen für WiFi-Scan
String topic_gpio_state_pub;     // Topic zum Veröffentlichen des aktuellen GPIO-Status
String topic_gpio_get_sub;       // Topic zum Abonnieren von Anfragen für den GPIO-Status
String topic_gpio_set_sub;       // Topic zum Abonnieren von Befehlen zur GPIO-Steuerung
String topic_settings_get_sub;   // Topic zum Abonnieren von Anfragen für die Einstellungen
String topic_settings_pub;       // Topic zum Veröffentlichen der Einstellungen
String topic_settings_set_sub;   // Topic zum Abonnieren von Befehlen zur Einstellung der
                                 // Geräteeinstellungen
String topic_settings_reset_sub; // Topic zum Abonnieren von Befehlen zum Zurücksetzen der Einstellungen

// Globale Variablen für den nicht-blockierenden Scan
bool wifiScanning = false; // Flag, ob ein WiFi-Scan läuft

// ----------------------------------------
// Timer für periodische Aufgaben
// Verwendet millis() für nicht-blockierende Zeitintervalle
// ----------------------------------------
long lastHeartbeatTime = 0;           // Zeitpunkt des letzten Heartbeats
const long heartbeatInterval = 30000; // Intervall für Heartbeats (30 Sekunden)

long lastWifiScanTime = 0; // Zeitpunkt des letzten WiFi-Scans

// Instanzen für die WLAN- und MQTT-Kommunikation
WiFiClient espClient;           // Der TCP-Client, der die WLAN-Verbindung verwaltet
PubSubClient client(espClient); // Der MQTT-Client, der über espClient kommuniziert

static void writePinValueToJson(JsonObject& pinObj, const Pin& pin) {
  if (std::holds_alternative<bool>(pin.value)) {
    pinObj["state"] = std::get<bool>(pin.value);
  } else if (std::holds_alternative<int>(pin.value)) {
    pinObj["state"] = std::get<int>(pin.value);
  } else if (std::holds_alternative<float>(pin.value)) {
    pinObj["state"] = std::get<float>(pin.value);
  } else if (std::holds_alternative<DigitalOutputState>(pin.value)) {
    pinObj["state"] =
        std::get<DigitalOutputState>(pin.value) == DigitalOutputState::HIGH_STATE ? "HIGH" : "LOW";
  } else {
    pinObj["state"] = nullptr;
  }
}

// ----------------------------------------
// Funktion: sendDeviceSettings
// Sendet die aktuellen Geräteeinstellungen als JSON an topic_settings_pub.
// ----------------------------------------
void sendDeviceSettings() {
  DynamicJsonDocument doc(512); // Ausreichend für alle Einstellungen

  doc["deviceName"] = device.deviceName;             // Aktueller Gerätename
  doc["wifiScanInterval"] = device.wifiScanInterval; // Der aktuell aktive Wert

  // GPIO Metadaten anhängen
  JsonArray gpioArray = doc.createNestedArray("gpioConfigs");
  for (const auto& [pinKey, pin] : device.pins) {
    JsonObject g = gpioArray.createNestedObject();
    g["pinNumber"] = pin.pinNumber;
    g["mode"] = pinModeToString(pin.currentMode);
    g["label"] = pin.label;
  }

  String payload;
  serializeJson(doc, payload);

  Serial.print("Sende Geräteeinstellungen: ");
  Serial.println(payload);

  if (client.connected()) {
    client.publish(topic_settings_pub.c_str(), payload.c_str());
  } else {
    Serial.println("MQTT Client ist NICHT verbunden, Einstellungen nicht gesendet.");
  }
}

// ----------------------------------------
// Funktion: updateDeviceSettings
// Aktualisiert Geräteeinstellungen basierend auf einer JSON-Nachricht.
// Speichert die Änderungen und sendet den aktualisierten Zustand zurück.
// ----------------------------------------
void updateDeviceSettings(String payloadString) {
  DynamicJsonDocument doc(512);

  DeserializationError error = deserializeJson(doc, payloadString);

  if (error) {
    Serial.print(F("JSON-Parsing für Settings fehlgeschlagen: "));
    Serial.println(error.f_str());
    return;
  }

  bool settingsChanged = false;

  if (doc.containsKey("deviceName")) {
    std::string newName = doc["deviceName"] | "";
    if (newName != device.deviceName) {
      bool isChanged = device.setDeviceName(newName);
      if (isChanged) {
        Serial.print("Gerätename aktualisiert zu: ");
        Serial.println(device.deviceName.c_str());
        settingsChanged = true;
      }
    }
  }

  if (doc.containsKey("wifiScanInterval")) {
    uint32_t newInterval = doc["wifiScanInterval"].as<uint32_t>();
    // Intervall muss mindestens 5 Sekunden sein und muss sich vom aktuellen Wert unterscheiden
    if (newInterval > 5000 && newInterval != device.wifiScanInterval) {
      bool isChanged = device.setWifiScanInterval(newInterval);
      if (isChanged) {
        Serial.print("WiFi Scan Intervall aktualisiert zu: ");
        Serial.println(device.wifiScanInterval);
        settingsChanged = true;
      }
    } else {
      Serial.print("Ungültiges oder unverändertes WiFi Scan Intervall: ");
      Serial.println(newInterval);
    }
  }

  // Aktualisiere GPIO-Metadaten falls vorhanden
  if (doc.containsKey("gpioConfigs") && doc["gpioConfigs"].is<JsonArray>()) {
    JsonArray gpioArray = doc["gpioConfigs"].as<JsonArray>();

    for (JsonObject newGpioValues : gpioArray) {
      bool pinChanged = false;

      int pinNum = newGpioValues["pinNumber"].as<int>();

      auto it = device.pins.find(pinNum);
      if (it == device.pins.end()) {
        Serial.print("Unbekannte pinNumber in GPIO Metadaten: ");
        Serial.println(pinNum);
        continue;
      }

      if (newGpioValues.containsKey("mode")) {
        std::string newMode = newGpioValues["mode"] | "None";
        PinMode mode = device.pins[pinNum].convertStringToPinMode(newMode);
        bool isChanged = device.pins[pinNum].setMode(mode);
        if (isChanged) {
          Serial.print("GPIO Pin ");
          Serial.print(pinNum);
          Serial.print(" Mode aktualisiert zu: ");
          Serial.println(newMode.c_str());
          pinChanged = true;
        }
      }
      if (newGpioValues.containsKey("label")) {
        std::string newLabel = newGpioValues["label"] | "";
        if (newLabel != device.pins[pinNum].label) {
          bool isChanged = device.pins[pinNum].setLabel(newLabel);
          if (isChanged) {
            Serial.print("GPIO Pin ");
            Serial.print(pinNum);
            Serial.print(" Label aktualisiert zu: ");
            Serial.println(newLabel.c_str());
            pinChanged = true;
          }
        }
      }

      if (pinChanged) {
        Serial.print("GPIO Metadaten für Pin ");
        Serial.print(pinNum);
        Serial.println(" aktualisiert.");
        settingsChanged = true;
      }
    }
  }

  // Nach der Aktualisierung die neuen Einstellungen zurücksenden und speichern,
  // damit das Frontend weiß, dass die Änderung übernommen wurde.
  if (settingsChanged) {
    // Speichere die aktualisierten Einstellungen auf LittleFS
    if (saveSettings()) {
      Serial.println("Einstellungen wurden auf LittleFS gespeichert!");
    } else {
      Serial.println("Fehler beim Speichern der Einstellungen auf LittleFS!");
    }
    sendDeviceSettings();
  }
}

// ----------------------------------------
// GPIO-Steuerung und Definitionen
// Definiert die GPIO-Pins, die im Projekt verwendet und gesteuert/überwacht werden
// ----------------------------------------
// Pins auf dem ESP32-DevKitC V4:
// Vermeide GPIOs 0, 1, 3, 5, 6-11 (Flash-Pins), 12 (Boot-Pin), 14 (Boot-Pin), 15 (Boot-Pin)

Device getDevicePreset() {
  if (String(BOARD_TYPE) == "ESP32_Dev_Kit_C_V4") {
    return ESP32_Dev_Kit_C_V4;
  }
  return ESP32_Custom;
}

Device device; // Initialisiere die Pin-Definitionen basierend auf dem Board-Typ

// #define PIN_2 2
// #define PIN_4 4
// #define PIN_16 16
// #define PIN_17 17

// Array speichert die aktuellen logischen Zustände (HIGH/LOW) der steuerbaren Pins
// int gpio_states[4] = {LOW, LOW, LOW, LOW};
// Array der tatsächlich verwendeten GPIO-Nummern. Reihenfolge muss gpio_states entsprechen.
// int control_pins[] = {PIN_2, PIN_4, PIN_16, PIN_17};
// Anzahl der definierten Pins
// const int NUM_PINS = sizeof(control_pins) / sizeof(control_pins[0]);

// GPIO Metadaten (Label/Group) - wird in littlefs_settings.h persistiert
// GPIOConfig gpioConfigs[NUM_PINS];

// ----------------------------------------
// Funktion: MQTT Callback
// Wird aufgerufen, wenn eine Nachricht auf einem abonnierten Topic empfangen wird
// ----------------------------------------
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Nachricht empfangen auf Topic: [");
  Serial.print(topic);
  Serial.print("] Payload: ");

  // Konvertiere das Payload (byte array) zu einem String für einfachere Verarbeitung
  String payloadString = "";
  for (int i = 0; i < length; i++) {
    payloadString += (char)payload[i];
  }
  Serial.println(payloadString);

  // Unterscheidung der eingehenden Nachrichten basierend auf dem Topic
  // -------------------------------------------------------------------

  // 1. Wenn ein GPIO-Steuerbefehl empfangen wird (z.B. Frontend schaltet Pin)
  if (String(topic) == topic_gpio_set_sub) {
    DynamicJsonDocument doc(512); // ArduinoJson Dokument für das Payload

    // Versuche, das Payload als JSON zu parsen
    DeserializationError error = deserializeJson(doc, payloadString);

    // Fehlerbehandlung beim JSON-Parsen
    if (error) {
      Serial.print(F("JSON-Parsing fehlgeschlagen: "));
      Serial.println(error.f_str());
      return; // Ungültige JSON-Nachricht, Funktion beenden
    }

    // Iteriere über jedes GPIO-Steuerobjekt im empfangenen JSON-Array
    // Format: [{"pinNumber": 2, "state": "ON"}, {"pinNumber": 4, "state": "OFF"}]
    for (JsonObject pinObj : doc.as<JsonArray>()) {
      // Extrahiere die Pin-Nummer und den Zustand aus dem Objekt
      if (!pinObj.containsKey("pinNumber") || !pinObj.containsKey("state")) {
        Serial.println("Fehler: Fehlende Felder 'pinNumber' oder 'state' in GPIO-Befehl");
        continue; // Diesen Befehl überspringen
      }

      int pinNum = pinObj["pinNumber"].as<int>(); // Extrahiere die Pin-Nummer
      String stateStr =
          pinObj["state"].as<String>(); // Extrahiere den Zustand ("ON", "OFF", "1", "0")

      int newState = 0; // Standardmäßig 0
      // Konvertiere den String-Zustand in 1/0
      if (stateStr == "ON" || stateStr == "1" || stateStr == "HIGH") {
        newState = 1;
      } else if (stateStr == "OFF" || stateStr == "0" || stateStr == "LOW") {
        newState = 0;
      } else {
        Serial.print("Unbekannter Zustand für Pin ");
        Serial.print(pinNum);
        Serial.print(": ");
        Serial.println(stateStr);
        continue; // Diesen Pin überspringen und nächsten Pin in der JSON verarbeiten
      }

      int setValueSuccessfully = device.pins[pinNum].setValue(
          newState == 1 ? DigitalOutputState::HIGH_STATE : DigitalOutputState::LOW_STATE);
      if (!setValueSuccessfully) {
        Serial.print("Fehler: Pin ");
        Serial.print(pinNum);
        Serial.println(
            " ist nicht im Digital_Output Modus oder existiert nicht. Befehl ignoriert.");
        continue; // Diesen Pin überspringen
      } else {
        Serial.print("Pin ");
        Serial.print(pinNum);
        Serial.print(" erfolgreich auf ");
        Serial.println(newState == 1 ? "HIGH" : "LOW");
      }
    }
    // Nach der Verarbeitung aller Befehle den aktualisierten GPIO-Status an das Frontend senden
    reportGpioStates();
  }
  // 2. Wenn eine Status-Anfrage empfangen wird (z.B. vom Frontend beim Laden)
  else if (String(topic) == topic_status_get_sub || String(topic) == topic_status_get_all_sub) {
    Serial.println("Anfrage empfangen auf /status/get Topic. Sende Status-Daten...");
    sendHeartbeat(); // Sende einen Heartbeat mit aktuellen Statusinformationen
  }
  // 3. Wenn eine WiFi-Scan-Anfrage empfangen wird
  else if (String(topic) == topic_wifi_get_sub) {
    Serial.println("Anfrage empfangen auf /wifi/get Topic. Führe WiFi-Scan durch...");
    performWifiScan(); // Führt einen WiFi-Scan durch und sendet die Ergebnisse
  }
  // 4. Wenn eine GPIO-Status-Anfrage empfangen wird
  else if (String(topic) == topic_gpio_get_sub) {
    Serial.println("Anfrage empfangen auf /gpio/get Topic. Sende GPIO-Zustände...");
    reportGpioStates(); // Sende den aktuellen Status aller GPIOs
  } else if (String(topic) == topic_settings_get_sub) {
    Serial.println("Anfrage empfangen auf /settings/get Topic. Sende aktuelle Einstellungen...");
    sendDeviceSettings(); // Funktion zum Senden der aktuellen Einstellungen
  }
  // NEU: 6. Wenn ein Befehl zur Einstellung der Geräte-Settings empfangen wird
  else if (String(topic) == topic_settings_set_sub) {
    Serial.println("Befehl empfangen auf /settings/set Topic. Aktualisiere Einstellungen...");
    updateDeviceSettings(payloadString); // Funktion zum Aktualisieren der Einstellungen
  }
  // Für alle anderen Topics, die abonniert sind, aber nicht explizit behandelt werden
  else if (String(topic) == topic_settings_reset_sub) {
    Serial.println("Befehl empfangen auf /settings/reset Topic. Lösche gespeicherte Einstellungen...");
    if (deleteSettings()) {
      Serial.println("Gespeicherte Einstellungen gelöscht. Gerät wird mit Standard-Einstellungen neu gestartet.");
      ESP.restart(); // Neustart des ESP32, damit die Standard-Einstellungen geladen werden
    } else {
      Serial.println("Fehler beim Löschen der gespeicherten Einstellungen!");
    }
  }
  else {
    Serial.print("Unbehandeltes Topic: ");
    Serial.println(topic);
  }
}

// ----------------------------------------
// Funktion: setup_wifi
// Verbindet den ESP32 mit dem konfigurierten WLAN-Netzwerk
// ----------------------------------------
void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Verbinde mit WLAN: ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  // Deaktiviert den WiFi Power Save Mode, um die MQTT-Verbindung stabiler zu halten
  WiFi.setSleep(WIFI_PS_NONE); // WICHTIG für stabile MQTT-Verbindung während WiFi-Operationen

  // Warten, bis die WLAN-Verbindung hergestellt ist
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WLAN verbunden!");
  Serial.print("IP Adresse: ");
  Serial.println(WiFi.localIP());
}

// ----------------------------------------
// Funktion: reconnect_mqtt
// Stellt die Verbindung zum MQTT-Broker her oder wieder her.
// Abonniert alle notwendigen Topics nach erfolgreicher Verbindung.
// ----------------------------------------
void reconnect_mqtt() {
  // Schleife, solange keine Verbindung zum Broker besteht
  while (!client.connected()) {
    Serial.print("Versuche MQTT-Verbindung...");

    // Versuche, eine Verbindung zum MQTT-Broker herzustellen
    if (client.connect(deviceId.c_str(),          // Client-ID (eindeutige Geräte-ID)
                       mqtt_user,                 // MQTT-Benutzername
                       mqtt_pass,                 // MQTT-Passwort
                       topic_status_pub.c_str(),  // willTopic
                       0,                         // willQoS
                       true,                      // willRetain
                       "{\"status\":\"offline\"}" // willMessage
                       )) {
      Serial.println("verbunden!");

      // Sende sofort nach dem Connect eine "online"-Statusnachricht (LWT wird überschrieben)
      sendHeartbeat(); // sendHeartbeat() sendet die "online" Statusnachricht

      // WICHTIG: Hier alle Topics abonnieren, die dieser ESP32 empfangen soll.
      // Diese Subscriptions gehen verloren, wenn die Verbindung abbricht und müssen
      // bei jedem Reconnect erneuert werden.
      client.subscribe(topic_gpio_set_sub.c_str());   // Abonnieren für GPIO-Steuerbefehle
      client.subscribe(topic_status_get_sub.c_str()); // Abonnieren für Status-Anfragen
      client.subscribe(
          topic_status_get_all_sub.c_str());        // Abonnieren für Status-Anfragen aller Geräte
      client.subscribe(topic_wifi_get_sub.c_str()); // Abonnieren für WiFi-Scan-Anfragen
      client.subscribe(topic_gpio_get_sub.c_str()); // Abonnieren für GPIO-Status-Anfragen
      client.subscribe(topic_settings_get_sub.c_str()); // Abonnieren für Settings-Anfragen
      client.subscribe(topic_settings_set_sub.c_str()); // Abonnieren für Setting-Änderungsbefehle
      client.subscribe(topic_settings_reset_sub.c_str()); // Abonnieren für Settings-Reset-Befehle

      // Initialen GPIO-Status senden (für Dashboard-Initialisierung)
      reportGpioStates();
    } else {
      Serial.print("Fehlgeschlagen, rc=");
      Serial.print(client.state()); // Zeigt den Fehlercode des MQTT-Clients
      Serial.println(" -> Erneuter Versuch in 5 Sekunden");
      delay(5000); // 5 Sekunden warten, bevor erneut versucht wird
    }
  }
}

// ----------------------------------------
// Funktion: sendHeartbeat
// Sendet den aktuellen Status des ESP32 als JSON-Nachricht.
// ----------------------------------------
void sendHeartbeat() {
  Serial.println("Sende Heartbeat...");
  lastHeartbeatTime = millis(); // Aktualisiert den Zeitpunkt des letzten Heartbeats

  DynamicJsonDocument doc(
      512); // ArduinoJson Dokument für den Heartbeat-Payload (512 Bytes für mehr Puffer)

  doc["status"] = "online";              // Status des Geräts
  doc["wifi"] = WiFi.SSID();             // Aktuell verbundenes WLAN-SSID
  doc["rssi"] = WiFi.RSSI();             // Signalstärke des verbundenen WLANs
  doc["uptime"] = millis() / 1000;       // Uptime in Sekunden
  doc["deviceName"] = device.deviceName; // Name des Geräts

  // Erstellt das GPIO-States-Array im neuen Format
  JsonArray gpio_states_json = doc.createNestedArray("gpioStates");

  // Fügt den Status jedes Pins als Objekt zum JSON-Array hinzu
  for (const auto& [pinKey, pin] : device.pins) {
    JsonObject pinObj = gpio_states_json.createNestedObject();
    pinObj["pinNumber"] = pin.pinNumber;
    writePinValueToJson(pinObj, pin);
    pinObj["mode"] = pinModeToString(pin.currentMode);
    pinObj["label"] = pin.label;
  }

  String payload;
  serializeJson(doc, payload); // Serialisiert das JSON-Dokument in einen String

  Serial.print("Heartbeat Payload: ");
  Serial.println(payload);

  if (client.connected()) {
    client.publish(topic_status_pub.c_str(),
                   payload.c_str(),
                   true); // Veröffentlicht die Nachricht (retained = true)
  } else {
    Serial.println("MQTT Client ist NICHT verbunden, Heartbeat nicht gesendet.");
  }
}

// ----------------------------------------
// Funktion: performWifiScan
// Führt einen WiFi-Scan durch und sendet die Ergebnisse als JSON.
// Nutzt einen nicht-blockierenden Scan-Mechanismus.
// ----------------------------------------
void performWifiScan() {
  if (wifiScanning)
    return; // Nicht starten, wenn bereits ein Scan läuft

  Serial.println("Starte nicht-blockierenden WLAN-Scan...");
  lastWifiScanTime = millis(); // Aktualisiert den Zeitpunkt des letzten Scans

  // WiFi.scanNetworks(true) startet einen nicht-blockierenden Scan im Hintergrund
  int n = WiFi.scanNetworks(true); // 'true' für asynchronen/nicht-blockierenden Scan
  if (n == -1) {                   // -1 bedeutet, Scan wurde gestartet, aber ist noch nicht fertig
    wifiScanning = true;
    Serial.println("WLAN-Scan im Hintergrund gestartet.");
  } else if (n == -2) { // -2 bedeutet, dass der Scan bereits läuft (sollte durch 'if(wifiScanning)'
                        // abgefangen werden)
    Serial.println("WLAN-Scan läuft bereits.");
  } else { // >= 0, Scan ist aus irgendeinem Grund sofort fertig (unwahrscheinlich bei
           // nicht-blockierendem Scan)
    Serial.println("WLAN-Scan unerwartet sofort fertig.");
    processWifiScanResults(n); // Ergebnisse direkt verarbeiten
  }
}

// ----------------------------------------
// Funktion: processWifiScanResults
// Verarbeitet die Ergebnisse eines abgeschlossenen WiFi-Scans.
// Wird vom loop() aufgerufen, wenn WiFi.scanComplete() fertig meldet.
// ----------------------------------------
void processWifiScanResults(int n) {
  Serial.print("WLAN-Scan abgeschlossen. Gefundene Netzwerke: ");
  Serial.println(n);

  // Prüfen, ob Netzwerke gefunden wurden
  if (n <= 0) {
    Serial.print("Keine Netzwerke gefunden oder Fehler beim Scan: ");
    Serial.println(n);
    if (client.connected()) {
      client.publish(topic_wifi_scan_pub.c_str(), "{\"networks\":[]}"); // Sende leeres JSON-Array
    } else {
      Serial.println("MQTT Client ist NICHT verbunden, WiFi Scan nicht gesendet.");
    }
  } else {
    // DynamicJsonDocument mit ausreichender Kapazität für Scan-Ergebnisse
    // 2048 Bytes sind eine gute Schätzung für bis zu 15-20 Netzwerke.
    DynamicJsonDocument doc(2048);

    JsonObject root = doc.to<JsonObject>();                  // Erstellt das Wurzelobjekt des JSON
    JsonArray networks = root.createNestedArray("networks"); // Erstellt ein Array für die Netzwerke

    // Fügt jedes gefundene Netzwerk als Objekt zum JSON-Array hinzu
    for (int i = 0; i < n; ++i) {
      JsonObject network = networks.createNestedObject();
      network["ssid"] = WiFi.SSID(i);                 // SSID des Netzwerks
      network["rssi"] = WiFi.RSSI(i);                 // Signalstärke
      network["encryption"] = WiFi.encryptionType(i); // Verschlüsselungstyp (als Zahl)
    }

    String json_output;
    serializeJson(doc, json_output); // Serialisiert das JSON-Dokument in einen String

    Serial.print("Heap nach JSON-Erstellung: ");
    Serial.println(ESP.getFreeHeap()); // Debug-Ausgabe des freien Heaps
    Serial.print("Größe der JSON-Nachricht: ");
    Serial.print(json_output.length()); // Debug-Ausgabe der Nachrichtengröße
    Serial.println(" Bytes");

    if (client.connected()) {
      Serial.println("MQTT Client ist verbunden, sende WiFi Scan.");
      client.publish(topic_wifi_scan_pub.c_str(),
                     json_output.c_str()); // Veröffentlicht die Nachricht
    } else {
      Serial.println("MQTT Client ist NICHT verbunden, WiFi Scan nicht gesendet.");
    }
  }

  WiFi.scanDelete();    // Scan-Ergebnisse löschen, um Speicher freizugeben und Heap zu entlasten
  wifiScanning = false; // Setzt das Flag zurück, da der Scan abgeschlossen ist
}

// ----------------------------------------
// Funktion: reportGpioStates
// Sendet den aktuellen Status aller konfigurierten GPIO-Pins als JSON.
// ----------------------------------------
void reportGpioStates() {
  // Dynamisches JSON-Dokument für GPIO-Zustände (256 Bytes sind ausreichend)
  DynamicJsonDocument doc(256);
  JsonObject root = doc.to<JsonObject>(); // Erstellt das Wurzelobjekt
  JsonArray gpio_states_json = root.createNestedArray("gpioStates");

  // Fügt den Status jedes Pins als Objekt zum JSON-Array hinzu
  for (const auto& [pinKey, pin] : device.pins) {
    JsonObject pinObj = gpio_states_json.createNestedObject();
    pinObj["pinNumber"] = pin.pinNumber;
    writePinValueToJson(pinObj, pin);
    pinObj["mode"] = pinModeToString(pin.currentMode);
    pinObj["label"] = pin.label;
  }

  String payload;
  serializeJson(doc, payload); // Serialisiert das JSON-Dokument in einen String

  Serial.print("Sende GPIO-Zustände: ");
  Serial.println(payload);

  if (client.connected()) {
    client.publish(topic_gpio_state_pub.c_str(), payload.c_str()); // Veröffentlicht die Nachricht
  } else {
    Serial.println("MQTT Client ist NICHT verbunden, GPIO-Zustände nicht gesendet.");
  }
}

// ----------------------------------------
// SETUP-Funktion
// Wird einmal beim Start des ESP32 ausgeführt.
// Initialisiert WLAN, MQTT, GPIOs und generiert die Geräte-ID.
// ----------------------------------------
void setup() {
  Serial.begin(115200); // Serielle Ausgabe starten für Debugging
  Serial.println("Setup starting...");
  Serial.print("Board Type aus secrets: ");
  Serial.println(BOARD_TYPE);
  device = getDevicePreset(); // Initialisiere die Pin-Definitionen basierend auf dem Board-Typ
  Serial.print("Board Type aus Device: ");
  Serial.println(device.type.c_str());
  // ========== LittleFS Initialisierung ==========
  if (!initLittleFS()) {
    Serial.println("KRITISCHER FEHLER: LittleFS konnte nicht initialisiert werden!");
    // Wir fahren trotzdem fort, verwenden aber Standard-Einstellungen
  } else {
    // Versuche, die gespeicherten Einstellungen zu laden
    if (!loadSettings()) {
      Serial.println("Keine gespeicherten Einstellungen gefunden, verwende Standards.");
      device.setDevicePinsToStandard(); // Setzt die Pins auf die Standardwerte basierend auf dem
                                        // Board-Typ
    }
  }

  // Debug-Ausgabe der geladenen Settings
  printSettings();

  // Debug-Ausgabe der Secret-Werte (optional, nur für Entwicklung)
  Serial.print("WLAN SSID: ");
  Serial.println(ssid);
  Serial.print("MQTT Broker IP: ");
  Serial.println(mqtt_broker);
  Serial.print("MQTT Username: ");
  Serial.println(mqtt_user);
  Serial.print("MQTT Password: ");
  Serial.println(mqtt_pass);
  Serial.print("Base Device Name: ");
  Serial.println(device.deviceName.c_str());

  // --- Generiere die eindeutige Device ID ---
  // Initialisiere WiFi im STA-Modus, um die MAC-Adresse auslesen zu können
  WiFi.mode(WIFI_STA);
  delay(100); // Gib dem WiFi-Modul Zeit zur Initialisierung

  // Holt die 6-Byte MAC-Adresse des WiFi-Moduls
  uint8_t mac[6];
  WiFi.macAddress(mac);

  // Formatiert die MAC-Adresse in einen 12-stelligen Hex-String ohne Trennzeichen
  char macStr[18]; // Puffer für "XXXXXXXXXXXX\0" (13 Zeichen)
  sprintf(macStr, "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

  deviceId = String(macStr);
  Serial.print("Generated Device ID: ");
  Serial.println(deviceId);
  // --- Ende Generierung ---

  // --- MQTT Topics initialisieren ---
  topic_status_get_all_sub = "esp32/all/status/get"; // Gemeinsames Topic für alle Geräte
  // Alle Topics basieren auf der generierten eindeutigen deviceId
  topic_status_pub = "esp32/" + deviceId + "/status";
  topic_wifi_scan_pub = "esp32/" + deviceId + "/wifi/scan";
  topic_gpio_state_pub = "esp32/" + deviceId + "/gpio/state";
  topic_gpio_set_sub = "esp32/" + deviceId + "/gpio/set";
  // Topics für spezifische Anfragen vom Frontend
  topic_status_get_sub = "esp32/" + deviceId + "/status/get";
  topic_wifi_get_sub = "esp32/" + deviceId + "/wifi/get";
  topic_gpio_get_sub = "esp32/" + deviceId + "/gpio/get";
  // Topics für Geräteeinstellungen
  topic_settings_get_sub = "esp32/" + deviceId + "/settings/get";
  topic_settings_pub = "esp32/" + deviceId + "/settings";
  topic_settings_set_sub = "esp32/" + deviceId + "/settings/set";
  topic_settings_reset_sub = "esp32/" + deviceId + "/settings/reset";

  // Debug-Ausgabe der generierten Topics zur Überprüfung
  Serial.print("MQTT Topic Heartbeat: ");
  Serial.println(topic_status_pub);
  Serial.print("MQTT Topic WiFi Scan: ");
  Serial.println(topic_wifi_scan_pub);
  Serial.print("MQTT Topic GPIO State: ");
  Serial.println(topic_gpio_state_pub);
  Serial.print("MQTT Topic GPIO Set (Sub): ");
  Serial.println(topic_gpio_set_sub);
  Serial.print("MQTT Topic Status Get (Sub): ");
  Serial.println(topic_status_get_sub);
  Serial.print("MQTT Topic WiFi Get (Sub): ");
  Serial.println(topic_wifi_get_sub);
  Serial.print("MQTT Topic GPIO Get (Sub): ");
  Serial.println(topic_gpio_get_sub);
  Serial.print("MQTT Topic Settings Get (Sub): ");
  Serial.println(topic_settings_get_sub);
  Serial.print("MQTT Topic Settings Publish: ");
  Serial.println(topic_settings_pub);
  Serial.print("MQTT Topic Settings Set (Sub): ");
  Serial.println(topic_settings_set_sub);
  Serial.print("MQTT Topic Settings Reset (Sub): ");
  Serial.println(topic_settings_reset_sub);
  // --- Ende Topics Initialisierung ---

  setup_wifi(); // Stellt die WLAN-Verbindung her

  // MQTT-Client konfigurieren
  client.setServer(mqtt_broker, mqtt_port); // Setzt die Broker-Adresse
  client.setCallback(callback); // Registriert die Callback-Funktion für eingehende Nachrichten
  client.setBufferSize(2048);   // Erhöht den internen MQTT-Puffer für größere Payloads
}

// ----------------------------------------
// LOOP-Funktion
// Wird kontinuierlich nach setup() ausgeführt.
// Verwaltet MQTT-Verbindung, verarbeitet periodische Aufgaben und Keep-Alives.
// ----------------------------------------
void loop() {
  // Stellt sicher, dass die MQTT-Verbindung aktiv ist, verbindet sich bei Bedarf neu.
  // Dies ist essenziell, damit der ESP32 Nachrichten senden und empfangen kann.
  if (!client.connected()) {
    reconnect_mqtt();
  }
  // client.loop() muss regelmäßig aufgerufen werden, um MQTT-Nachrichten zu verarbeiten,
  // Keep-Alive-Pakete zu senden und die Verbindung aufrechtzuerhalten.
  // Wenn diese Funktion zu lange nicht aufgerufen wird, kann die Verbindung abbrechen.
  client.loop();

  // --- Handling für den nicht-blockierenden WiFi-Scan ---
  // Überprüft, ob ein WLAN-Scan im Hintergrund läuft
  if (wifiScanning) {
    // WiFi.scanComplete() gibt die Anzahl der gefundenen Netzwerke zurück,
    // wenn der Scan abgeschlossen ist, oder -1, wenn er noch läuft.
    int scanStatus = WiFi.scanComplete();
    if (scanStatus >= 0) { // Der Scan ist abgeschlossen (0 oder mehr Netzwerke gefunden)
      processWifiScanResults(scanStatus); // Verarbeite die Ergebnisse
      // Das 'wifiScanning' Flag wird in processWifiScanResults auf 'false' gesetzt.
    }
  }
  // --- Ende Handling WiFi-Scan ---

  unsigned long currentMillis = millis(); // Aktuelle Uptime in Millisekunden

  // Periodischer Heartbeat senden
  // Sendet einen Heartbeat, wenn die Zeit seit dem letzten Heartbeat abgelaufen ist.
  if (currentMillis - lastHeartbeatTime >= heartbeatInterval) {
    sendHeartbeat();
  }

  // Periodischer WiFi-Scan starten
  // Startet einen neuen WiFi-Scan, wenn die Zeit seit dem letzten Scan abgelaufen ist
  // und kein anderer Scan gerade läuft.
  if (!wifiScanning && currentMillis - lastWifiScanTime >= device.wifiScanInterval) {
    performWifiScan();
  }

  // Kleine Pause (1 Millisekunde)
  // Dies gibt dem ESP32-Scheduler Zeit, andere interne Aufgaben zu erledigen (z.B. WiFi-Stack).
  // Es ist wichtig, die loop() nicht zu blockieren, um die Stabilität zu gewährleisten.
  delay(1);
}