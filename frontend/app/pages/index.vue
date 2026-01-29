<script lang="ts" setup>
const { t } = useI18n();

useHead({
  title: t("common.appTitle"),
});
</script>

<template>
  <div class="h-full flex flex-col">
    <TopNav show-nav/>
    <Devices class="flex-grow overflow-auto custom-scrollbar" />
  </div>
</template>
