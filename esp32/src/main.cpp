#include "secrets.h"      // Enthält vertrauliche WLAN- und MQTT-Zugangsdaten. MUSS in .gitignore!
#include <ArduinoJson.h>  // Bibliothek für effizientes JSON-Parsing und -Generierung
#include <WiFi.h>         // Bibliothek für WLAN-Funktionalität
#include <PubSubClient.h> // Bibliothek für MQTT-Kommunikation
#include <Esp.h>          // Für ESP-spezifische Funktionen wie ESP.getFreeHeap()

// ----------------------------------------
// 1. WLAN-Einstellungen
// Werden aus der secrets.h-Datei geladen, um sie vom Code zu trennen
// ----------------------------------------
const char* ssid = WIFI_SSID;       // WLAN-SSID
const char* password = WIFI_PASSWORD; // WLAN-Passwort

// ----------------------------------------
// 2. MQTT-Einstellungen
// Werden ebenfalls aus der secrets.h-Datei geladen
// ----------------------------------------
const char* mqtt_broker = MQTT_BROKER_IP;   // IP-Adresse des MQTT-Brokers (dein Docker-Host)
const int mqtt_port = MQTT_BROKER_PORT;     // MQTT-Port des Brokers (Standard 1883)
const char* mqtt_user = MQTT_USERNAME;      // MQTT-Benutzername
const char* mqtt_pass = MQTT_PASSWORD;      // MQTT-Passwort

// Globale Variablen für die Geräte-ID und MQTT-Topics.
// Diese werden dynamisch in setup() initialisiert, da deviceId von der MAC abhängt.
String deviceId;                  // Eindeutige ID für diesen ESP32 (Basisname + MAC-Adresse)
String topic_status_pub;          // Topic zum Veröffentlichen des Online-Status/Heartbeats
String topic_status_get_sub;      // Topic zum Abonnieren von Anfragen für den Status
String topic_wifi_scan_pub;       // Topic zum Veröffentlichen von WiFi-Scan-Ergebnissen
String topic_wifi_get_sub;        // Topic zum Abonnieren von Anfragen für WiFi-Scan
String topic_gpio_state_pub;      // Topic zum Veröffentlichen des aktuellen GPIO-Status
String topic_gpio_get_sub;        // Topic zum Abonnieren von Anfragen für den GPIO-Status
String topic_gpio_set_sub;        // Topic zum Abonnieren von Befehlen zur GPIO-Steuerung

// Instanzen für die WLAN- und MQTT-Kommunikation
WiFiClient espClient;             // Der TCP-Client, der die WLAN-Verbindung verwaltet
PubSubClient client(espClient);   // Der MQTT-Client, der über espClient kommuniziert

// ----------------------------------------
// 3. GPIO-Steuerung und Definitionen
// Definiert die GPIO-Pins, die im Projekt verwendet und gesteuert/überwacht werden
// ----------------------------------------
// Pins auf dem ESP32-DevKitC V4:
// Vermeide GPIOs 0, 1, 3, 5, 6-11 (Flash-Pins), 12 (Boot-Pin), 14 (Boot-Pin), 15 (Boot-Pin)
// GPIO 2 ist oft eine integrierte LED. Andere sind gut für allgemeine Zwecke.
#define LED_PIN 2         // Beispiel: Eine LED oder ein digitaler Output
#define RELAY_PIN_1 4     // Beispiel: Ein Relais oder anderer digitaler Output
#define RELAY_PIN_2 16    // Beispiel: Ein weiterer digitaler Output
#define SENSOR_GPIO 17    // Beispiel: Ein Pin, der seinen Zustand meldet, aber nicht gesteuert wird

// Array speichert die aktuellen logischen Zustände (HIGH/LOW) der steuerbaren Pins
int gpio_states[4] = {LOW, LOW, LOW, LOW};
// Array der tatsächlich verwendeten GPIO-Nummern. Reihenfolge muss gpio_states entsprechen.
int control_pins[] = {LED_PIN, RELAY_PIN_1, RELAY_PIN_2, SENSOR_GPIO};
// Anzahl der definierten Pins
const int NUM_PINS = sizeof(control_pins) / sizeof(control_pins[0]);


// ----------------------------------------
// 4. Timer für periodische Aufgaben
// Verwendet millis() für nicht-blockierende Zeitintervalle
// ----------------------------------------
long lastHeartbeatTime = 0;       // Zeitpunkt des letzten Heartbeats
const long heartbeatInterval = 30000; // Intervall für Heartbeats (30 Sekunden)

long lastWifiScanTime = 0;        // Zeitpunkt des letzten WiFi-Scans
const long wifiScanInterval = 60000;  // Intervall für WiFi-Scans (60 Sekunden)

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

    // Iteriere über jedes Pin-State-Paar im empfangenen JSON (z.B. {"2":"ON", "4":"OFF"})
    for (JsonPair p : doc.as<JsonObject>()) {
      int pinNum = String(p.key().c_str()).toInt(); // Extrahiere die Pin-Nummer aus dem JSON-Key
      String stateStr = p.value().as<String>();     // Extrahiere den Zustand ("ON", "OFF", "1", "0")

      int newState = LOW; // Standardmäßig LOW
      // Konvertiere den String-Zustand in HIGH/LOW
      if (stateStr == "ON" || stateStr == "1" || stateStr == "HIGH") {
        newState = HIGH;
      } else if (stateStr == "OFF" || stateStr == "0" || stateStr == "LOW") {
        newState = LOW;
      } else {
        Serial.print("Unbekannter Zustand für Pin ");
        Serial.print(pinNum);
        Serial.print(": ");
        Serial.println(stateStr);
        continue; // Diesen Pin überspringen und nächsten Pin in der JSON verarbeiten
      }

      // Suche den Pin im control_pins-Array, um dessen Index zu finden und Zustand zu aktualisieren
      bool pinFound = false;
      for (int i = 0; i < NUM_PINS; i++) {
        if (control_pins[i] == pinNum) {
          digitalWrite(pinNum, newState); // Pin physisch schalten
          gpio_states[i] = newState;      // Internen Zustand aktualisieren
          Serial.print("GPIO ");
          Serial.print(pinNum);
          Serial.print(" auf ");
          Serial.println(newState == HIGH ? "HIGH" : "LOW");
          pinFound = true;
          break; // Pin gefunden, Schleife beenden
        }
      }
      if (!pinFound) {
        Serial.print("Befehl für unbekannten oder nicht steuerbaren Pin empfangen: ");
        Serial.println(pinNum);
      }
    }
    // Nach der Verarbeitung aller Befehle den aktualisierten GPIO-Status an das Frontend senden
    reportGpioStates();
  }
  // 2. Wenn eine Status-Anfrage empfangen wird (z.B. vom Frontend beim Laden)
  else if (String(topic) == topic_status_get_sub) {
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
  }
  // Für alle anderen Topics, die abonniert sind, aber nicht explizit behandelt werden
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
    
    // Setzt die Last Will and Testament (LWT) Nachricht
    // Diese wird vom Broker gesendet, falls der ESP32 unerwartet offline geht.
    // 'online' wird beim erfolgreichen Connect gesendet, 'offline' bei unerwartetem Disconnect.
    // client.setWill(topic_status_pub.c_str(), "{\"status\":\"offline\",\"reason\":\"unexpected_disconnect\"}", true, 0);

    // Versuche, eine Verbindung zum MQTT-Broker herzustellen
    if (client.connect(deviceId.c_str(), mqtt_user, mqtt_pass)) {
      Serial.println("verbunden!");

      // Sende sofort nach dem Connect eine "online"-Statusnachricht (LWT wird überschrieben)
      sendHeartbeat(); // sendHeartbeat() sendet die "online" Statusnachricht

      // WICHTIG: Hier alle Topics abonnieren, die dieser ESP32 empfangen soll.
      // Diese Subscriptions gehen verloren, wenn die Verbindung abbricht und müssen
      // bei jedem Reconnect erneuert werden.
      client.subscribe(topic_gpio_set_sub.c_str());     // Abonnieren für GPIO-Steuerbefehle
      client.subscribe(topic_status_get_sub.c_str());   // Abonnieren für Status-Anfragen
      client.subscribe(topic_wifi_get_sub.c_str());     // Abonnieren für WiFi-Scan-Anfragen
      client.subscribe(topic_gpio_get_sub.c_str());     // Abonnieren für GPIO-Status-Anfragen

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

  DynamicJsonDocument doc(256); // ArduinoJson Dokument für den Heartbeat-Payload (256 Bytes sind ausreichend)

  doc["status"] = "online";      // Status des Geräts
  doc["wifi"] = WiFi.SSID();     // Aktuell verbundenes WLAN-SSID
  doc["rssi"] = WiFi.RSSI();     // Signalstärke des verbundenen WLANs
  doc["uptime"] = millis() / 1000; // Uptime in Sekunden

  String payload;
  serializeJson(doc, payload); // Serialisiert das JSON-Dokument in einen String

  Serial.print("Heartbeat Payload: ");
  Serial.println(payload);

  if (client.connected()) {
    client.publish(topic_status_pub.c_str(), payload.c_str(), true); // Veröffentlicht die Nachricht (retained = true)
  } else {
    Serial.println("MQTT Client ist NICHT verbunden, Heartbeat nicht gesendet.");
  }
}

// ----------------------------------------
// Funktion: performWifiScan
// Führt einen WiFi-Scan durch und sendet die Ergebnisse als JSON.
// ----------------------------------------
void performWifiScan() {
  lastWifiScanTime = millis(); // Aktualisiert den Zeitpunkt des letzten Scans
  Serial.println("Suche WLAN-Netzwerke...");
  int n = WiFi.scanNetworks(); // Führt den Scan durch

  // Prüfen, ob der Scan erfolgreich war
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
    // Kann je nach SSID-Länge und Anzahl der Netzwerke angepasst werden.
    DynamicJsonDocument doc(2048); 

    JsonObject root = doc.to<JsonObject>();            // Erstellt das Wurzelobjekt des JSON
    JsonArray networks = root.createNestedArray("networks"); // Erstellt ein Array für die Netzwerke

    // Fügt jedes gefundene Netzwerk als Objekt zum JSON-Array hinzu
    for (int i = 0; i < n; ++i) {
      JsonObject network = networks.createNestedObject();
      network["ssid"] = WiFi.SSID(i);         // SSID des Netzwerks
      network["rssi"] = WiFi.RSSI(i);         // Signalstärke
      network["encryption"] = WiFi.encryptionType(i); // Verschlüsselungstyp (als Zahl)
    }

    String json_output;
    serializeJson(doc, json_output); // Serialisiert das JSON-Dokument in einen String

    Serial.print("Heap nach JSON-Erstellung: "); Serial.println(ESP.getFreeHeap()); // Debug-Ausgabe des freien Heaps
    Serial.print("Größe der JSON-Nachricht: "); Serial.print(json_output.length()); // Debug-Ausgabe der Nachrichtengröße
    Serial.println(" Bytes");

    if (client.connected()) {
      Serial.println("MQTT Client ist verbunden, sende WiFi Scan.");
      client.publish(topic_wifi_scan_pub.c_str(), json_output.c_str()); // Veröffentlicht die Nachricht
    } else {
      Serial.println("MQTT Client ist NICHT verbunden, WiFi Scan nicht gesendet.");
    }
  }

  WiFi.scanDelete(); // Scan-Ergebnisse löschen, um Speicher freizugeben und Heap zu entlasten
}

// ----------------------------------------
// Funktion: reportGpioStates
// Sendet den aktuellen Status aller konfigurierten GPIO-Pins als JSON.
// ----------------------------------------
void reportGpioStates() {
  // Dynamisches JSON-Dokument für GPIO-Zustände (256 Bytes sind ausreichend)
  DynamicJsonDocument doc(256);
  JsonObject gpio_states_json = doc.to<JsonObject>(); // Erstellt das Wurzelobjekt

  // Fügt den Status jedes Pins zum JSON-Objekt hinzu
  for (int i = 0; i < NUM_PINS; i++) {
    gpio_states_json[String(control_pins[i])] = gpio_states[i]; // Pin-Nummer als Key, Zustand als Value (0/1)
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

  // Debug-Ausgabe der Secret-Werte (optional, nur für Entwicklung)
  Serial.print("WLAN SSID: "); Serial.println(ssid);
  Serial.print("MQTT Broker IP: "); Serial.println(mqtt_broker);
  Serial.print("MQTT Username: "); Serial.println(mqtt_user);
  Serial.print("MQTT Password: "); Serial.println(mqtt_pass);
  Serial.print("Base Device Name: "); Serial.println(BASE_DEVICE_NAME);

  // GPIO Pins als OUTPUT konfigurieren und initialen Zustand auf LOW setzen
  for (int i = 0; i < NUM_PINS; i++) {
    pinMode(control_pins[i], OUTPUT);
    digitalWrite(control_pins[i], LOW);
    gpio_states[i] = LOW; // Internen Zustand initialisieren
  }

  // --- Generiere die eindeutige Device ID ---
  // Holt die 6-Byte MAC-Adresse des WiFi-Moduls
  uint8_t mac[6];
  WiFi.macAddress(mac);

  // Formatiert die MAC-Adresse in einen 12-stelligen Hex-String ohne Trennzeichen
  char macStr[18]; // Puffer für "XXXXXXXXXXXX\0" (13 Zeichen)
  sprintf(macStr, "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

  // Kombiniert den Basisnamen aus secrets.h mit der MAC-Adresse
  deviceId = String(BASE_DEVICE_NAME) + "-" + String(macStr);
  Serial.print("Generated Device ID: "); Serial.println(deviceId);
  // --- Ende Generierung ---

  // --- MQTT Topics dynamisch initialisieren ---
  // Alle Topics basieren auf der generierten eindeutigen deviceId
  topic_status_pub = "esp32/" + deviceId + "/status";
  topic_wifi_scan_pub = "esp32/" + deviceId + "/wifi/scan";
  topic_gpio_state_pub = "esp32/" + deviceId + "/gpio/state";
  topic_gpio_set_sub = "esp32/" + deviceId + "/gpio/set";
  // Topics für spezifische Anfragen vom Frontend
  topic_status_get_sub = "esp32/" + deviceId + "/status/get";
  topic_wifi_get_sub = "esp32/" + deviceId + "/wifi/get";
  topic_gpio_get_sub = "esp32/" + deviceId + "/gpio/get";

  // Debug-Ausgabe der generierten Topics zur Überprüfung
  Serial.print("MQTT Topic Heartbeat: "); Serial.println(topic_status_pub);
  Serial.print("MQTT Topic WiFi Scan: "); Serial.println(topic_wifi_scan_pub);
  Serial.print("MQTT Topic GPIO State: "); Serial.println(topic_gpio_state_pub);
  Serial.print("MQTT Topic GPIO Set (Sub): "); Serial.println(topic_gpio_set_sub);
  Serial.print("MQTT Topic Status Get (Sub): "); Serial.println(topic_status_get_sub);
  Serial.print("MQTT Topic WiFi Get (Sub): "); Serial.println(topic_wifi_get_sub);
  Serial.print("MQTT Topic GPIO Get (Sub): "); Serial.println(topic_gpio_get_sub);
  // --- Ende Topics Initialisierung ---


  setup_wifi(); // Stellt die WLAN-Verbindung her

  // MQTT-Client konfigurieren
  client.setServer(mqtt_broker, mqtt_port); // Setzt die Broker-Adresse
  client.setCallback(callback);             // Registriert die Callback-Funktion für eingehende Nachrichten
  client.setBufferSize(2048);               // Erhöht den internen MQTT-Puffer für größere Payloads
}

// ----------------------------------------
// LOOP-Funktion
// Wird kontinuierlich nach setup() ausgeführt.
// Verwaltet MQTT-Verbindung, verarbeitet periodische Aufgaben und Keep-Alives.
// ----------------------------------------
void loop() {
  // Stellt sicher, dass die MQTT-Verbindung aktiv ist, verbindet sich bei Bedarf neu
  if (!client.connected()) {
    reconnect_mqtt();
  }
  client.loop(); // Muss regelmäßig aufgerufen werden, um MQTT-Nachrichten zu verarbeiten und Keep-Alive-Pakete zu senden

  unsigned long currentMillis = millis(); // Aktuelle Uptime in Millisekunden

  // Periodischer Heartbeat senden
  if (currentMillis - lastHeartbeatTime >= heartbeatInterval) {
    sendHeartbeat();
  }

  // Periodischer WiFi-Scan durchführen
  if (currentMillis - lastWifiScanTime >= wifiScanInterval) {
    performWifiScan();
  }

  delay(1); // Kleine Pause, um den Watchdog-Timer nicht auszulösen und andere Tasks zuzulassen
}