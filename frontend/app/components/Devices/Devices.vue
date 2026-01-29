<script lang="ts" setup>
import {
  GPIOSubTopic,
  MessageTopic,
  WifiSubTopic,
  type StatusMessage,
  type WifiScanMessage,
} from "~/models/message";
import ESP32 from "./ESP32.vue";
import type {
  ContentTab,
  Device,
  ExtendedGPIO,
  GPIO,
  GPIOGroupId,
  GPIOPin,
  GPIOPinState,
  SetGPIO,
} from "~/models/device";

import {
  Settings,
  PanelsLeftBottom,
  Wifi,
  Cpu,
  type LucideProps,
} from "lucide-vue-next";
import type { FunctionalComponent } from "vue";

const { $mqtt, $mqttConnectionState } = useNuxtApp();

const { t } = useI18n();

const {
  devices,
  addDevice,
  initializeStore,
  updateDeviceLastSeen,
  addStatusMessage,
  addWifiScanMessage,
  addGpioStateMessage,
  saveDataIntoLocalStorage,
  loadDataFromLocalStorage,
} = useDeviceStore();

const activeTab = ref<ContentTab>("overview");
const tabs: {
  label: string;
  value: ContentTab;
  icon: FunctionalComponent<LucideProps>;
  activeClasses?: string;
}[] = [
  {
    label: t("device.tabs.overview"),
    value: "overview",
    icon: PanelsLeftBottom,
    activeClasses: "text-success",
  },
  {
    label: t("device.tabs.wifi"),
    value: "wifi",
    icon: Wifi,
    activeClasses: "text-secondary",
  },
  {
    label: t("device.tabs.gpio"),
    value: "gpio",
    icon: Cpu,
    activeClasses: "text-warning",
  },
  {
    label: t("device.tabs.settings"),
    value: "settings",
    icon: Settings,
    activeClasses: "text-error",
  },
];

const currentDeviceId = ref<string | null>(null);
const currentDevice = computed(() => {
  return (
    devices.value.find((device) => device.id === currentDeviceId.value) || null
  );
});

// Wifi
const wifiScanMessages = computed(() => {
  return (
    currentDevice.value?.messages
      .find((msg) => msg.topic === MessageTopic.WIFI)
      ?.messages.filter((m) => m.supTopic === WifiSubTopic.SCAN) || []
  );
});

onMounted(() => {
  loadDataFromStorage();
  devices.value.forEach((device) => {
    device.lastSeen = null;
  });

  if (!$mqtt) {
    console.error("MQTT Client not injected by plugin.");
    return;
  }

  $mqtt.on("message", (topic, message) => {
    const deviceId = topic.split("/")[1]; // Extrahiere die Geräte-ID aus dem Topic
    const topicType = topic.split("/")[2]; // Extrahiere den Nachrichtentyp aus dem Topic
    const subTopicType = topic.split("/")[3]; // Extrahiere den Sub-Nachrichtentyp aus dem Topic (falls vorhanden)

    if (!deviceId || !topicType) return;
    let deviceEntry = devices.value.find(({ id }) => id === deviceId);

    if (!deviceEntry) {
      const newDevice: Device = {
        id: deviceId,
        name: "",
        lastSeen: null,
        deviceStatus: "offline",
        gpios: [],
        messages: [],
      };
      addDevice(newDevice);
      deviceEntry = devices.value.find(({ id }) => id === deviceId);
    }
    if (!deviceEntry) return console.error("Device Entry should exist here.");
    updateDeviceLastSeen(deviceId, Date.now());

    switch (topicType) {
      case MessageTopic.STATUS:
        const statusMessage: StatusMessage = JSON.parse(message.toString());
        statusMessage.timestamp = Date.now();
        if (statusMessage.deviceName)
          deviceEntry.name = statusMessage.deviceName;
        deviceEntry.deviceStatus = statusMessage.status;
        addStatusMessage(deviceId, statusMessage);

        break;

      case MessageTopic.WIFI:
        if (subTopicType === WifiSubTopic.SCAN) {
          const wifiScanMessage: WifiScanMessage = {
            supTopic: WifiSubTopic.SCAN,
            networks: JSON.parse(message.toString()).networks,
            timestamp: Date.now(),
          };
          addWifiScanMessage(deviceId, wifiScanMessage);
        } else console.warn("SubTopicType not supported: ", subTopicType);

        break;

      case MessageTopic.GPIO:
        if (subTopicType === GPIOSubTopic.STATE) {
          const gpioStateMessage = {
            supTopic: GPIOSubTopic.STATE as const,
            gpioStates: JSON.parse(message.toString()).gpioStates,
            timestamp: Date.now(),
          };

          addGpioStateMessage(deviceId, gpioStateMessage);
        } else console.warn("SubTopicType not supported: ", subTopicType);

        break;

      case MessageTopic.SETTINGS:
        break;

      default:
        console.warn("Unknown topic type:", topicType);
    }
    saveDataIntoLocalStorage();
  });

  getStatus("all");
});

onUnmounted(() => {
  $mqtt.removeAllListeners("message");
});

function setGpioPinState(deviceId: string, pin: GPIOPin, value: GPIOPinState) {
  const payload: SetGPIO[] = [{ pinNumber: pin, state: value }];
  const topic = `esp32/${deviceId}/gpio/set`;
  const message = JSON.stringify(payload);
  $mqtt.publish(topic, message);
}

function getStatus(deviceId: string) {
  const topic = `esp32/${deviceId}/status/get`;
  $mqtt.publish(topic, "");
}

function getWifiScan(deviceId: string) {
  const topic = `esp32/${deviceId}/wifi/get`;
  $mqtt.publish(topic, "");
}

function getGpioStates(deviceId: string) {
  const topic = `esp32/${deviceId}/gpio/get`;
  $mqtt.publish(topic, "");
}

function loadDataFromStorage() {
  const data = loadDataFromLocalStorage();
  if (data) {
    initializeStore(JSON.parse(data));
  }
}

interface GPIOGroup {
  groupId: GPIOGroupId;
  gpios: ExtendedGPIO[];
}

const gpioGroups = computed(() => {
  const gpios = devices.value.flatMap((device) =>
    device.gpios.map((gpio) => ({
      ...gpio,
      deviceId: device.id,
      deviceName: device.name,
      deviceStatus: device.deviceStatus,
    })),
  );
  const groups: GPIOGroup[] = [];
  gpios.forEach((gpio) => {
    if (!gpio.group) gpio.group = "none";
    let group = groups.find((g) => g.groupId === gpio.group);
    if (!group) {
      group = { groupId: gpio.group, gpios: [] };
      groups.push(group);
    }
    group.gpios.push(gpio);
  });

  return groups;
});

function changeTab(tab: ContentTab) {
  if (devices.value?.[0]) currentDeviceId.value = devices.value[0].id;
  activeTab.value = tab;
}
</script>

<template>
  <div class="flex flex-col items-center h-full">
    <div class="w-full flex flex-grow p-24 overflow-auto custom-scrollbar">
      <div v-if="activeTab === 'overview'">
        <template v-for="(group, index) in gpioGroups" :key="index">
          <div class="w-full flex flex-wrap justify-center">
            <template v-for="gpio in group.gpios" :key="gpio.pinNumber">
              <GPIOActorUniversal
                :gpio="gpio"
                class="m-8"
                @setGpioPin="
                  ({ deviceId, pin, value }) =>
                    setGpioPinState(deviceId, pin, value)
                "
              />
            </template>
          </div>
        </template>
      </div>

      <template v-else>
        <BasicCard class="flex h-full w-full">
          <div class="w-[200px] p-16 border-r border-primary ">
            <h2>
              {{ t("common.devices") }}
            </h2>
            <ul>
              <li
                v-for="device in devices"
                :key="device.id"
                @click="currentDeviceId = device.id"
              >
                <button @click="currentDeviceId = device.id">
                  {{ device.name }}
                </button>
              </li>
            </ul>
          </div>

          <div class="flex-grow overflow-hidden">
            <DevicesSectionsWiFiHistory
              v-if="currentDevice"
              :device-id="currentDevice.id"
              :wi-fi-scan-messages="wifiScanMessages"
            />
          </div>
        </BasicCard>
      </template>

      <!-- <template v-for="device in devices" :key="device.id">
        <ESP32
          :device="device"
          :clientState="$mqttConnectionState"
          :activeTab="activeTab"
          @setGpioPin="
            ({ pin, value }) => setGpioPinState(device.id, pin, value)
          "
          @getStatus="getStatus(device.id)"
          @getWifiScan="getWifiScan(device.id)"
          @getGpioStates="getGpioStates(device.id)"
        />
      </template> -->
    </div>

    <div class="flex gap-16 p-16">
      <template v-for="tab in tabs" :key="tab.value">
        <BasicCard class="h-[100px] w-[100px] flex justify-center items-center">
          <BasicCardButton
            :isActive="activeTab === tab.value"
            :is-selectable="true"
            general-classes="w-full h-full"
            :activeClasses="tab.activeClasses + ' pointer-events-none'"
            @click="changeTab(tab.value)"
          >
            <template #top>
              <span class="text-12 text-primary">{{
                t(`device.tabs.${tab.value}`)
              }}</span>
            </template>
            <Component :is="tab.icon" :size="24" />
          </BasicCardButton>
        </BasicCard>
      </template>
    </div>
  </div>
</template>

<style scoped></style>
