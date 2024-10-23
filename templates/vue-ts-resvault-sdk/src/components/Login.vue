<template>
  <div class="page-container">
    <div class="form-container">
      <h2 class="heading">Resilient App</h2>

      <div ref="animationContainer" class="animation-container"></div>

      <div class="form-group text-center mb-4">
        <label class="signin-label">Sign In Via</label>
        <button
          type="button"
          class="btn btn-secondary oauth-button"
          @click="handleAuthentication"
        >
          <div class="logoBox">
            <img :src="resvaultLogo" alt="ResVault" class="oauth-logo" />
          </div>
          <span class="oauth-text">ResVault</span>
        </button>
      </div>
    </div>

    <NotificationModal
      v-model="showModal"
      :title="modalTitle"
      :message="modalMessage"
    />

    <Loader v-if="isLoadingAfterLogin" />
  </div>
</template>

<script lang="ts">
  import { defineComponent, onMounted, ref, onBeforeUnmount, computed } from 'vue';
  import ResVaultSDK from 'resvault-sdk';
  import NotificationModal from './NotificationModal.vue';
  import Loader from './Loader.vue';
  import { v4 as uuidv4 } from 'uuid';
  import lottie from 'lottie-web';
  import animation from '../assets/images/animation.json';
  import resvaultLogo from '../assets/images/resilientdb.svg';
  import { useStore } from 'vuex';
  import { useRouter } from 'vue-router';

  export default defineComponent({
    name: 'Login',
    components: {
      NotificationModal,
      Loader,
    },
    setup() {
      const store = useStore();
      const router = useRouter();
      const sdk = ref<ResVaultSDK | null>(null);
      const showModal = ref(false);
      const modalTitle = ref('');
      const modalMessage = ref('');
      const animationContainer = ref<HTMLElement | null>(null);

      const isLoadingAfterLogin = computed(() => store.getters.isLoadingAfterLogin);

      if (!sdk.value) {
        sdk.value = new ResVaultSDK();
      }

      const handleAuthentication = () => {
        if (sdk.value) {
          sdk.value.sendMessage({
            type: 'login',
            direction: 'login',
          });
        } else {
          showErrorModal('Error', 'SDK is not initialized.');
        }
      };

      const showErrorModal = (title: string, message: string) => {
        modalTitle.value = title;
        modalMessage.value = message;
        showModal.value = true;
        store.dispatch('stopLoading');
      };

      const handleMessage = (event: MessageEvent) => {
        const message = event.data;
        if (
          message &&
          message.type === 'FROM_CONTENT_SCRIPT' &&
          message.data
        ) {
          if (typeof message.data === 'object') {
            if (message.data.success === true) {
              const token = uuidv4();
              store.dispatch('login', token);
            } else if (message.data.success === false) {
              showErrorModal(
                'Authentication Failed',
                'Please connect ResVault to this ResilientApp and try again.'
              );
            }
          } else if (message.data === 'error') {
            showErrorModal(
              'Authentication Failed',
              'Please connect ResVault to this ResilientApp and try again.'
            );
          }
        }
      };

      onMounted(() => {
        if (sdk.value) {
          sdk.value.addMessageListener(handleMessage);
        }

        if (animationContainer.value) {
          const instance = lottie.loadAnimation({
            container: animationContainer.value,
            renderer: 'svg',
            loop: true,
            autoplay: true,
            animationData: animation,
          });

          const observer = new IntersectionObserver((entries) => {
            entries.forEach((entry) => {
              if (entry.isIntersecting) {
                instance.play();
              } else {
                instance.pause();
              }
            });
          });

          observer.observe(animationContainer.value);

          onBeforeUnmount(() => {
            instance.destroy();
            observer.disconnect();
          });
        }
      });

      onBeforeUnmount(() => {
        if (sdk.value) {
          sdk.value.removeMessageListener(handleMessage);
        }
      });

      return {
        resvaultLogo,
        animationContainer,
        handleAuthentication,
        showModal,
        modalTitle,
        modalMessage,
        isLoadingAfterLogin,
      };
    },
  });
</script>