<script lang="ts" setup>
const { t } = useI18n();

useHead({
  title: t("common.appTitle"),
});
</script>

<template>
  <div class="flex flex-col gap-48 pb-48">
    <TopNav show-nav/>
    <Devices />
  </div>
</template>
