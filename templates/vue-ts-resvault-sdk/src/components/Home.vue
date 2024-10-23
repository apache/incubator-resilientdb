<template>
    <div>
      <Login v-if="!isAuthenticated" />
      <TransactionForm v-else />
    </div>
</template>
  
<script lang="ts">
  import { defineComponent, computed } from 'vue';
  import { useStore } from 'vuex';
  import Login from './Login.vue';
  import TransactionForm from './TransactionForm.vue';
  
  export default defineComponent({
    name: 'Home',
    components: {
      Login,
      TransactionForm,
    },
    setup() {
      const store = useStore();
      const isAuthenticated = computed(() => store.getters.isAuthenticated);
  
      return {
        isAuthenticated,
      };
    },
  });
</script>
  