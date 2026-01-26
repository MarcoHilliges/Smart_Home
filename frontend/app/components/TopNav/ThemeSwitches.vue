<script lang="ts" setup>
import { Transition, TransitionGroup } from "vue";

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
  <BasicCard class="flex justify-center w-[150px]">
    <BasicCardButton
      v-for="mode in modes"
      :key="mode.value"
      class="border-r last:border-none"
      :is-active="themeModeInLocalStorage === mode.value"
      :is-selectable="themeModeInLocalStorage !== mode.value"
      general-classes="w-1/4"
      :active-classes="`!w-1/2 ${mode.activeClasses}`"
      @click="toggleThemeMode(mode.value)"
    >
      <component
        :is="mode.icon"
        :size="themeModeInLocalStorage === mode.value ? 24 : 12"
      />
    </BasicCardButton>
  </BasicCard>
</template>

<style lang="scss" scoped></style>
