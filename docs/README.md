# Dokumentation: IoT Dashboard mit ESP32

## 1. Projektbeschreibung
**Kurzbeschreibung:** Ein vollständiges IoT-System zur Überwachung und Steuerung mehrerer ESP32-Mikrocontrollers über ein modernes Web-Dashboard.
**Gelöste Probleme:** Vereinfachte Geräteüberwachung, Remote-Steuerung von GPIOs, Echtzeit-Statusupdates, dynamische WLAN-Analyse.

## 2. Systemarchitektur
* **Diagramm:** 
    **ESP32 (Mikrocontroller)** --MQTT--> **RabbitMQ (Docker) + MQTT/WebMQTT Plugin** --MQTT--> **Nuxt 4 App (Dashboard)**

* **Komponenten:**
    - **ESP32 Firmware:** Läuft auf dem Mikrocontroller, sammelt Daten (WLAN, GPIO), führt Befehle aus und kommuniziert über MQTT.
    - **MQTT Broker (RabbitMQ im Docker):** Der zentrale Knotenpunkt für die Nachrichtenkommunikation. Empfängt Daten vom ESP32 und leitet Befehle vom Dashboard weiter. Unterstützt MQTT über TCP und WebSockets.
    - **Nuxt 4 Frontend (Web-Dashboard):** Die Benutzeroberfläche, die Daten visualisiert, Steuerbefehle sendet und den Gesamtstatus des IoT-Systems anzeigt.
    - **Kommunikation:** Alle Komponenten kommunizieren über das MQTT-Protokoll. Der ESP32 nutzt TCP (Port 1883), das Nuxt-Frontend nutzt WebSockets (Port 15675).

## 3. Implementierte Features
* **ESP32:**
    - Stabile MQTT-Verbindung zum Broker (mit Reconnect-Logik).
    - Eindeutige Geräte-ID (Basisname + MAC).
    - Last Will and Testament (LWT) für robusten Online-/Offline-Status.
    - Periodischer/Anfrage-basierter WiFi-Scanner (nicht-blockierend, JSON-Output).
    - Periodischer/Anfrage-basierter Heartbeat mit Status und WLAN-Inf (JSON-Output).
    - GPIO-Steuerung über MQTT (JSON-Input).
    - GPIO-Status-Reporting (JSON-Output).
    - Anfrage-basierte Übermittlung aller Daten.
    - Einstellungsverwaltung (Abrufen und Setzen von wifiUpdateInterval).

* **Nuxt 4 Frontend:**
    - Responsives Design (Tailwind CSS).
    - MQTT-Verbindung zum Broker (über WebSockets).
    - Geräteübersicht: Liste aller verbundenen ESP32-Geräte.
    - Live Online/Offline/Timeout Statusanzeige.
    - Letzte Aktivität (Last Seen).
    - WiFi Scanner Ansicht: Anzeige gescannter Netzwerke (mit RSSI, Verschlüsselung), selektierbar nach Zeitstempel, sortierbar.
    - GPIO Control Panel: Übersicht über steuerbare Pins.
    - Tooltip für Status und WiFi-SSIDs.
    - Speicherung der Gerätedaten im LocalStorage für Persistenz über Reloads.
    - Statusabfrage für Geräte beim Start/Reload.
    - Einstellungsansicht für ESP32-Parameter (z.B. wifiUpdateInterval).
    - Dark/Light Mode (Speicherung der Einstellung im LocalStorage)
    - Sprachumschaltung (DE, EN)

## 4. Technologie-Stack
* **Frontend:**
    - Nuxt 4
    - Styling: Tailwind CSS
    - MQTT Client (Frontend): mqtt.js
    - Sprachumschaltung: i18n

* **Message Broker:**
    - RabbitMQ 4.2.2 (Docker) mit MQTT-Plugin

* **ESP32 Firmware:**
    - Arduino Framework (C++)
    - JSON-Parsing (ESP32): ArduinoJson v7.4.2

* **Containerisierung:**
    - Docker & Docker Compose

* **Entwicklungstools:**
    - Visual Studio Code
    - Arduino IDE

5. ## Installation & Setup
<details>
<summary>Installation & Setup</summary>

* **Voraussetzungen:**
    - **Docker Desktop:** Installiert und läuft (für Windows/macOS) oder Docker Engine (für Linux).
    - **Node.js (v18+):** Installiert (für Nuxt Frontend).
    - **npm / pnpm / yarn:** Installiert (für Nuxt Frontend).
    - **Arduino IDE:** Installiert (für ESP32 Firmware) mit ESP32 Board Manager und Bibliotheken (PubSubClient, ArduinoJson).
    - **Ein ESP32-DevKitC V4 Board:** Mit installiertem CP210x USB-Treiber.

* **A. Backend (RabbitMQ mit Docker Compose):**
    1. **Navigiere zum docker/ Verzeichnis:**

        ``` bash
        cd <Projekt-Root>/docker
        ```


    2. **`docker-compose.yml` bearbeiten:** Stelle sicher, dass die docker-compose.yml den RabbitMQ 4.x mit MQTT und WebMQTT Plugins konfiguriert:

        ``` yaml
        # docker/docker-compose.yml
        services:
        rabbitmq:
            image: rabbitmq:4-management-alpine
            hostname: rabbitmq-broker
            ports:
            - "5672:5672"
            - "1883:1883"
            - "15672:15672"
            - "15675:15675"
            environment:
            RABBITMQ_DEFAULT_USER: guest
            RABBITMQ_DEFAULT_PASS: guest
            volumes:
            - ./data/rabbitmq:/var/lib/rabbitmq
            - ./config/enabled_plugins:/etc/rabbitmq/enabled_plugins
            restart: unless-stopped
            ```

    3. **`config/enabled_plugins` erstellen:** Erstelle im `docker/` Verzeichnis einen `config/` Ordner und darin eine Datei namens enabled_plugins mit folgendem Inhalt:

        ``` text
        [rabbitmq_management,rabbitmq_mqtt,rabbitmq_web_mqtt].
        ```

    4. **Broker starten:**

        ``` bash
        docker compose up -d
        ```

    5. **Verifikation:** Überprüfe im Browser unter [http://localhost:15672](http://localhost:15672), ob das RabbitMQ Management UI erreichbar ist und unter "Overview" der http/web-mqtt Listener auf Port 15675 angezeigt wird.

* **B. ESP32 Firmware:**
    1. **Navigiere zum Firmware-Verzeichnis:**

        ``` bash
        cd <Projekt-Root>/esp32
        ```

    2. **secrets.h erstellen:** Kopiere `secrets.h.example` zu `secrets.h` und passe die Werte an:

        ``` c++

        // esp32/secrets.h
        #define WIFI_SSID           "DEINE_WLAN_SSID"
        #define WIFI_PASSWORD       "DEIN_WLAN_PASSWORT"
        #define MQTT_BROKER_IP      "IP_DES_DOCKER_HOSTS" // z.B. 192.168.1.100
        #define MQTT_BROKER_PORT    1883
        #define MQTT_USERNAME       "guest"
        #define MQTT_PASSWORD       "guest"
        #define BASE_DEVICE_NAME    "ESP32-Dashboard" // Benutzerdefinierbarer Name
        ```

    3. **Arduino IDE öffnen:** Öffne den .ino-Sketch im `esp32/` Verzeichnis.

    4. **Board und Port auswählen:** Wähle unter Tools > Board "ESP32 Dev Module" und den korrekten COM-Port deines ESP32 aus.

    5. **Kompilieren und Hochladen:** Klicke auf den Upload-Button. Halte bei Bedarf die "BOOT"-Taste am ESP32 gedrückt, bis der Upload beginnt.

    6. **Verifikation:** Öffne den seriellen Monitor (115200 Baud). Du solltest die WLAN-Verbindung und die MQTT-Verbindungsbestätigungen sehen, sowie die generierte Device ID.

* **C. Nuxt 4 Frontend:**

    1. **Navigiere zum Frontend-Verzeichnis:**

        ``` bash
        cd <Projekt-Root>/frontend/nuxt-iot-dashboard
        ```
    2. **.env Datei erstellen:** Kopiere im Root des Frontend-Projekts die `.env.example` zu `.env` und passe die Werte an:

        ``` dotenv
        # frontend/nuxt-iot-dashboard/.env
        NUXT_PUBLIC_MQTT_BROKER_HOST="IP_DES_DOCKER_HOSTS" # z.B. 192.168.1.100
        NUXT_PUBLIC_MQTT_BROKER_PORT=15675
        NUXT_PUBLIC_MQTT_BROKER_PATH="/ws"
        NUXT_PUBLIC_MQTT_USERNAME="guest"
        NUXT_PUBLIC_MQTT_PASSWORD="guest"
        ```

    3. **Abhängigkeiten installieren:**

        ``` bash
        pnpm install # oder npm install
        ```

    4. **Entwicklungs-Server starten:**

        ``` bash
        pnpm dev # oder npm run dev
        ```

    5. **Verifikation:** Öffne den Browser unter [http://localhost:3000](http://localhost:3000). Das Dashboard sollte den MQTT-Status "Connected" anzeigen und die Daten deines ESP32 empfangen.
</details>

## 6. Ordnerstruktur des Repositorys
<details>
<summary>Ordnerstruktur des Repositorys</summary>

    ```
    text
    Copy
    ├── docker/
    │   ├── docker-compose.yml
    │   ├── config/
    │   │   └── enabled_plugins
    │   └── .gitignore
    ├── docs/
    │   ├── Beschreibung ESP32 DevKit V4.pdf
    │   └── README.md
    ├── esp32/
    │   ├── src/
    │   │   ├── main.cpp
    │   │   ├── secrets.h
    │   │   └── secrets.h.example
    │   ├── .gitignore
    │   └── platformio.ini
    └── frontend/
        ├── app/
        │   ├── assets/
        │   ├── components/
        │   ├── composables/
        │   ├── models/
        │   ├── pages/
        │   ├── plugins/
        │   │   └── mqtt.client.ts
        │   └── app.vue
        ├── i18n/
        ├── public/
        ├── .env
        ├── .env.example
        ├── .gitignore
        ├── eslint.config.mjs
        ├── nuxt.config.mjs
        ├── package.json
        ├── pnpm-lock.yaml
        ├── README.md
        ├── tailwind.config.ts
        └── tsconfig.ts
    ```

</details>

## 7. Nutzung des Dashboards
- Nachdem alle Komponenten laufen, kannst du im Dashboard den Status deiner ESP32-Geräte sehen.
- Klicke auf die Tabs, um zwischen Geräte-Übersicht, WiFi-Scan-Historie, GPIO-Steuerung und Einstellungen zu wechseln.
- Nutze die Buttons im GPIO-Panel, um die Pins deines ESP32 zu schalten.

## 8. Verwendete AI-Tools
- ChatGPT 5: Genutzt für Code-Generierung, Debugging-Unterstützung und Strukturierung der Dokumentation.
- GitHub Copilot: Genutzt für Code-Vervollständigung und schnelle Syntax-Referenz.
- Workflow-Einfluss: Beschleunigte die Entwicklung und half bei der Fehlerdiagnose durch präzise Fehlermeldungs-Analyse.