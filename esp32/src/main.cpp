#include "secrets.h"

#include <ArduinoJson.h>
#include <WiFi.h>
#include <PubSubClient.h>

// ----------------------------------------
// 1. WLAN-Einstellungen
// ----------------------------------------
const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWORD;

// ----------------------------------------
// 2. MQTT-Einstellungen
// ----------------------------------------
// !!! WICHTIG: Ersetze dies durch die IP-Adresse deines PCs, auf dem Docker läuft !!!
const char* mqtt_broker = MQTT_BROKER_IP;
const int mqtt_port = MQTT_BROKER_PORT;
const char* mqtt_user = MQTT_USERNAME;
const char* mqtt_pass = MQTT_PASSWORD;

// Eindeutige ID für diesen ESP32 (wird zur Laufzeit generiert)
String deviceId;

// MQTT Topics (werden in setup() initialisiert)
String topic_status_pub;
String topic_wifi_scan_pub;
String topic_gpio_state_pub;
String topic_gpio_set_sub;

WiFiClient espClient; // Erstellt einen TCP-Client für die WLAN-Verbindung
PubSubClient client(espClient); // Erstellt den MQTT-Client mit dem TCP-Client

// ----------------------------------------
// 3. GPIO-Steuerung
// ----------------------------------------
// Beispiele für häufig verwendete GPIOs: 2, 4, 16, 17, 18, 19, 21, 22, 23, 25, 26, 27
// Vermeide GPIOs 0, 1, 3, 5, 6-11 (Flash-Pins), 12 (Boot-Pin), 14 (Boot-Pin), 15 (Boot-Pin) für einfache Verwendung
#define LED_PIN 2     // Beispiel-LED
#define RELAY_PIN_1 4
#define RELAY_PIN_2 16
#define SENSOR_GPIO 17 // Beispiel: Dieser Pin meldet nur seinen Zustand, wird nicht gesteuert

// Aktuelle Zustände der steuerbaren Pins (für Status-Reporting)
int gpio_states[4] = {LOW, LOW, LOW, LOW}; // Entspricht der Reihenfolge der Pins im Array unten
int control_pins[] = {LED_PIN, RELAY_PIN_1, RELAY_PIN_2, SENSOR_GPIO}; // Die zu steuernden/überwachenden Pins
const int NUM_PINS = sizeof(control_pins) / sizeof(control_pins[0]);


// ----------------------------------------
// 4. Timer für periodische Aufgaben
// ----------------------------------------
long lastHeartbeatTime = 0;
const long heartbeatInterval = 30000; // 30 Sekunden

long lastWifiScanTime = 0;
const long wifiScanInterval = 60000; // 60 Sekunden

// ----------------------------------------
// MQTT Callback-Funktion (für eingehende Nachrichten)
// ----------------------------------------
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Nachricht empfangen auf Topic: [");
  Serial.print(topic);
  Serial.print("] Payload: ");
  
  // Konvertiere Payload zu einem String für einfachere Verarbeitung
  String payloadString = "";
  for (int i = 0; i < length; i++) {
    payloadString += (char)payload[i];
  }
  Serial.println(payloadString);

  // Wenn der Befehl für GPIO-Steuerung empfangen wird
  if (String(topic) == topic_gpio_set_sub) {
    // --- JSON-Parsing mit ArduinoJson ---
    DynamicJsonDocument doc(512); // Eine moderate Kapazität für die Befehle
    
    // Parsen der JSON-Payload
    DeserializationError error = deserializeJson(doc, payloadString);

    // Prüfen, ob das Parsen erfolgreich war
    if (error) {
      Serial.print(F("JSON-Parsing fehlgeschlagen: "));
      Serial.println(error.f_str());
      return; // Funktion beenden, da keine gültige JSON-Nachricht
    }

    // Iteriere über die Schlüssel-Wert-Paare in der JSON-Nachricht
    // Erwarte Keys wie "2", "4" (Pinnummern als Strings) und Werte wie "ON", "OFF"
    for (JsonPair p : doc.as<JsonObject>()) {
      int pinNum = String(p.key().c_str()).toInt(); // Key (Pinnummer) als Integer
      String stateStr = p.value().as<String>();     // Wert (Zustand) als String

      int newState = LOW;
      if (stateStr == "ON" || stateStr == "1" || stateStr == "HIGH") {
        newState = HIGH;
      } else if (stateStr == "OFF" || stateStr == "0" || stateStr == "LOW") {
        newState = LOW;
      } else {
        Serial.print("Unbekannter Zustand für Pin ");
        Serial.print(pinNum);
        Serial.print(": ");
        Serial.println(stateStr);
        continue; // Nächsten Pin verarbeiten
      }

      // Finde den Pin im control_pins Array und aktualisiere seinen Zustand
      bool pinFound = false;
      for (int i = 0; i < NUM_PINS; i++) {
        if (control_pins[i] == pinNum) {
          digitalWrite(pinNum, newState);
          gpio_states[i] = newState; // Zustand im Array aktualisieren
          Serial.print("GPIO ");
          Serial.print(pinNum);
          Serial.print(" auf ");
          Serial.println(newState == HIGH ? "HIGH" : "LOW");
          pinFound = true;
          break; // Pin gefunden und verarbeitet
        }
      }
      if (!pinFound) {
        Serial.print("Befehl für unbekannten oder nicht steuerbaren Pin empfangen: ");
        Serial.println(pinNum);
      }
    }
    // Nach dem Verarbeiten aller Befehle den aktualisierten Status aller Pins senden
    reportGpioStates();
    // --- Ende JSON-Parsing ---
  }
}

// ----------------------------------------
// WLAN-Verbindungslogik
// ----------------------------------------
void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Verbinde mit WLAN: ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

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
// MQTT Reconnect-Logik
// ----------------------------------------
void reconnect_mqtt() {
  while (!client.connected()) {
    Serial.print("Versuche MQTT-Verbindung...");
    if (client.connect(deviceId.c_str(), mqtt_user, mqtt_pass)) {
      Serial.println("verbunden");
      // Nach erfolgreicher Verbindung:
      client.publish(topic_status_pub.c_str(), "online", true); // "online" mit retained flag
      client.subscribe(topic_gpio_set_sub.c_str()); // Steuer-Topic abonnieren

      // Initialen GPIO-Status senden
      reportGpioStates();

    } else {
      Serial.print("fehlgeschlagen, rc=");
      Serial.print(client.state());
      Serial.println(" -> Erneuter Versuch in 5 Sekunden");
      delay(5000);
    }
  }
}

// ----------------------------------------
// Funktionen für Pflicht-Features
// ----------------------------------------

// Heartbeat / Online-Status senden
void sendHeartbeat() {
  Serial.println("Sende Heartbeat: online");
  // Optional: JSON mit Uptime, Speicher etc.
  String payload = "{\"status\":\"online\",\"uptime\":" + String(millis()/1000) + "}";
  client.publish(topic_status_pub.c_str(), payload.c_str(), true); // Retained, falls Dashboard später startet
}

// WiFi-Scan durchführen und Ergebnisse senden
void performWifiScan() {
  Serial.println("Suche WLAN-Netzwerke...");
  int n = WiFi.scanNetworks();

  if (n <= 0) { // Fehler oder keine Netzwerke gefunden
    Serial.print("Keine Netzwerke gefunden oder Fehler beim Scan: ");
    Serial.println(n);
    if (client.connected()) {
      client.publish(topic_wifi_scan_pub.c_str(), "{\"networks\":[]}"); // Leeres JSON-Array senden
    } else {
      Serial.println("MQTT Client ist NICHT verbunden, WiFi Scan nicht gesendet.");
    }
    
  } else { // Netzwerke gefunden
    // 2048 Bytes sind eine gute, sichere Obergrenze für normale Nachrichten.
    DynamicJsonDocument doc(2048); // Versuche 2048 Bytes als Kapazität

    JsonObject root = doc.to<JsonObject>();
    JsonArray networks = root.createNestedArray("networks");

    for (int i = 0; i < n; ++i) {
      JsonObject network = networks.createNestedObject();
      network["ssid"] = WiFi.SSID(i);
      network["rssi"] = WiFi.RSSI(i);
      network["encryption"] = WiFi.encryptionType(i); 
    }

    // Erstelle den JSON-String
    String json_output;
    serializeJson(doc, json_output);

    Serial.print("Heap nach JSON-Erstellung: "); Serial.println(ESP.getFreeHeap());
    Serial.print("Größe der JSON-Nachricht: "); Serial.print(json_output.length()); 
    Serial.println(" Bytes");

    if (client.connected()) {
      Serial.println("MQTT Client ist verbunden, sende WiFi Scan.");
      client.publish(topic_wifi_scan_pub.c_str(), json_output.c_str());
    } else {
      Serial.println("MQTT Client ist NICHT verbunden, WiFi Scan nicht gesendet.");
    }
  }

  WiFi.scanDelete(); // Scan-Ergebnisse löschen, um Speicher freizugeben
}

// Aktuellen GPIO-Status aller Pins senden
void reportGpioStates() {
  String json = "{\"gpio_states\":{";
  for (int i = 0; i < NUM_PINS; i++) {
    json += "\"" + String(control_pins[i]) + "\":" + (gpio_states[i] == HIGH ? "1" : "0");
    if (i < NUM_PINS - 1) {
      json += ",";
    }
  }
  json += "}}";
  Serial.print("Sende GPIO-Zustände: ");
  Serial.println(json);
  client.publish(topic_gpio_state_pub.c_str(), json.c_str());
}

// ----------------------------------------
// SETUP-Funktion
// ----------------------------------------
void setup() {
  Serial.begin(115200); // Serielle Ausgabe starten für Debugging
  Serial.println("Setup starting...");

  // Debug-Ausgabe der Secret-Werte (optional, nur für Entwicklung)
  Serial.print("WLAN SSID: "); Serial.println(ssid);
  Serial.print("MQTT Broker IP: "); Serial.println(mqtt_broker);
  Serial.print("MQTT Username: "); Serial.println(mqtt_user);
  Serial.print("MQTT Password: "); Serial.println(mqtt_pass);
  Serial.print("Base Device Name: "); Serial.println(BASE_DEVICE_NAME); // Zeigt den Wert aus secrets.h


  // GPIO Pins als OUTPUT konfigurieren und initialen Zustand setzen
  for (int i = 0; i < NUM_PINS; i++) {
    pinMode(control_pins[i], OUTPUT);
    digitalWrite(control_pins[i], LOW);
    gpio_states[i] = LOW;
  }

  // --- Generiere die eindeutige Device ID ---
  uint8_t mac[6];
  WiFi.macAddress(mac); // Holt die 6-Byte MAC-Adresse

  char macStr[18]; // Puffer für "XX:XX:XX:XX:XX:XX\0" (17 Zeichen) oder "XXXXXXXXXXXX\0" (13 Zeichen)
  // Formatierung als "XXXXXXXXXXXX" ohne Doppelpunkte, um URL-freundlicher zu sein
  sprintf(macStr, "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

  deviceId = String(BASE_DEVICE_NAME) + "-" + String(macStr);
  // --- Ende Generierung ---

  // --- MQTT Topics dynamisch initialisieren ---
  topic_status_pub = "esp32/" + deviceId + "/status";
  topic_wifi_scan_pub = "esp32/" + deviceId + "/wifi/scan";
  topic_gpio_state_pub = "esp32/" + deviceId + "/gpio/state";
  topic_gpio_set_sub = "esp32/" + deviceId + "/gpio/set";
  
  // Debug-Ausgabe der generierten Topics
  Serial.print("MQTT Topic Heartbeat: "); Serial.println(topic_status_pub);
  Serial.print("MQTT Topic WiFi Scan: "); Serial.println(topic_wifi_scan_pub);
  Serial.print("MQTT Topic GPIO State: "); Serial.println(topic_gpio_state_pub);
  Serial.print("MQTT Topic GPIO Set (Sub): "); Serial.println(topic_gpio_set_sub);
  // --- Ende Topics Initialisierung ---


  setup_wifi(); // WLAN verbinden

  client.setServer(mqtt_broker, mqtt_port); // MQTT-Broker einstellen
  client.setCallback(callback); // Callback-Funktion für eingehende Nachrichten registrieren

  client.setBufferSize(2048); // Erhöhe den Puffer auf 2048 Bytes (oder mehr, falls nötig)
}

// ----------------------------------------
// LOOP-Funktion (wird wiederholt ausgeführt)
// ----------------------------------------
void loop() {
  // Sicherstellen, dass MQTT verbunden ist
  if (!client.connected()) {
    reconnect_mqtt();
  }
  client.loop(); // Muss regelmäßig aufgerufen werden, um Nachrichten zu empfangen und Keep-Alive zu senden

  unsigned long currentMillis = millis();

  // Heartbeat senden
  if (currentMillis - lastWifiScanTime < wifiScanInterval && currentMillis - lastHeartbeatTime >= heartbeatInterval) {
    lastHeartbeatTime = currentMillis;
    sendHeartbeat();
  }

    // WiFi-Scan durchführen
  if (currentMillis - lastWifiScanTime >= wifiScanInterval) {
    lastWifiScanTime = currentMillis;
    performWifiScan();
  }

  // Kleine Verzögerung, um den ESP32 nicht zu überlasten
  delay(1);
}