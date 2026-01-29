<script setup lang="ts">
import { Github, Linkedin } from "lucide-vue-next";

const { t } = useI18n();
</script>

<template>
  <div class="flex p-16 justify-center gap-16 border-t border-primary dark:border-primary-light">
    <NuxtLink
      to="https://github.com/MarcoHilliges"
      target="_blank"
      class="border border-primary dark:border-primary-light rounded-full p-4"
    >
      <Github :size="20" />
    </NuxtLink>

    <NuxtLink
      to="https://www.linkedin.com/in/marco-hilliges"
      target="_blank"
      class="border border-primary dark:border-primary-light rounded-full p-4"
    >
      <Linkedin :size="20" />
    </NuxtLink>

    <NuxtLink
      to="/imprint"
      class=""
    >
      {{ t('common.imprint') }}
    </NuxtLink>

        <NuxtLink
      to="/data-protection"
      class=""
    >
      {{ t('common.data-protection') }}
    </NuxtLink>
  </div>
</template>
