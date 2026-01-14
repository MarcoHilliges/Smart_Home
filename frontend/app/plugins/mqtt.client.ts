import mqtt from 'mqtt' 
import type { MqttClientState } from '~/models/message';

export default defineNuxtPlugin(nuxtApp => {
  const config = useRuntimeConfig();

  const mqttConnectionState = ref<MqttClientState>('disconnected');

  // Verwende die Variablen aus der Runtime Config
  const MQTT_BROKER_HOST = config.public.mqttBrokerHost;
  const MQTT_BROKER_PORT = Number(config.public.mqttBrokerPort);
  const MQTT_BROKER_PATH = config.public.mqttBrokerPath;
  const MQTT_USERNAME = config.public.mqttUsername;
  const MQTT_PASSWORD = config.public.mqttPassword;

  // Eindeutige Client ID für das Dashboard
  const CLIENT_ID = 'nuxt-dashboard-' + Math.random().toString(16).substring(2, 8); 

  // Verbindungskonfiguration für mqtt.js
  const client = mqtt.connect({
    host: MQTT_BROKER_HOST,
    port: MQTT_BROKER_PORT,
    path: MQTT_BROKER_PATH,
    username: MQTT_USERNAME,
    password: MQTT_PASSWORD,
    clientId: CLIENT_ID,
    protocol: 'ws',         // 'ws' für WebSockets im Browser
    reconnectPeriod: 5000,  // Alle 5 Sekunden reconnecten bei Verbindungsverlust
    keepalive: 5,          // Keepalive-Interval in Sekunden
  });

  client.on('connect', () => {
    console.log('MQTT Client connected to broker (manual plugin)!');

    client.subscribe("esp32/+/status");
    client.subscribe("esp32/+/wifi/scan");
    client.subscribe("esp32/+/gpio/state");
    mqttConnectionState.value = 'connected';
  });

  client.on('error', (error) => {
    console.error('MQTT Client connection error (manual plugin):', error);
    mqttConnectionState.value = 'error';
  });

  client.on('reconnect', () => {
    console.log('MQTT Client reconnecting...');
    mqttConnectionState.value = 'reconnecting';
  });

  client.on('close', () => {
    console.log('MQTT Client disconnected.');
    mqttConnectionState.value = 'disconnected';
  });

  return {
    provide: {
      mqtt: client, // Das Objekt wird unter $mqtt verfügbar sein
      mqttConnectionState: mqttConnectionState, // Der Verbindungsstatus wird unter $mqttConnectionState verfügbar sein
    }
  }
});