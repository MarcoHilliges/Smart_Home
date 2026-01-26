<script lang="ts" setup>
const {
  setThemeModeToDocument,
  toggleThemeMode,
  themeModeInLocalStorage,
  modes,
} = useThemeMode();

onBeforeMount(() => {
  setThemeModeToDocument(themeModeInLocalStorage.value);
});

watch(themeModeInLocalStorage, (newValue) => {
  setThemeModeToDocument(newValue);
});
</script>

<template>
  <div class="flex justify-center h-full">
    <button
      v-for="mode in modes"
      :key="mode.value"
      @click="toggleThemeMode(mode.value)"
      class="nav-icon-button"
      :class="{
        'is-active': themeModeInLocalStorage === mode.value,
        selectable: themeModeInLocalStorage !== mode.value,
      }"
    >
      <component :is="mode.icon" :size="16" />
    </button>
  </div>
</template>
