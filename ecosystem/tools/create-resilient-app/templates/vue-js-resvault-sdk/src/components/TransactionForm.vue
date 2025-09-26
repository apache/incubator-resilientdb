<!--
  Licensed to the Apache Software Foundation (ASF) under one
  or more contributor license agreements.  See the NOTICE file
  distributed with this work for additional information
  regarding copyright ownership.  The ASF licenses this file
  to you under the Apache License, Version 2.0 (the
  "License"); you may not use this file except in compliance
  with the License.  You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing,
  software distributed under the License is distributed on an
  "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
  KIND, either express or implied.  See the License for the
  specific language governing permissions and limitations
  under the License.
-->

<template>
    <div class="page-container">
      <div class="form-container">
        <div class="d-flex justify-content-between align-items-center mb-4">
          <h2 class="heading">Submit Transaction</h2>
          <button
            type="button"
            class="btn btn-danger logout-button"
            @click="handleLogout"
          >
            Logout
          </button>
        </div>
  
        <form @submit.prevent="handleSubmit">
          <div class="form-group mb-3">
            <input
              type="text"
              class="form-control"
              placeholder="Enter your amount here"
              v-model="amount"
            />
          </div>
  
          <div class="form-group mb-3">
            <input
              type="text"
              class="form-control"
              placeholder="Enter your data here (JSON)"
              v-model="data"
            />
          </div>
  
          <div class="form-group mb-4">
            <input
              type="text"
              class="form-control"
              placeholder="Enter recipient address here"
              v-model="recipient"
            />
          </div>
  
          <div class="form-group text-center">
            <button type="submit" class="btn btn-primary button">
              Submit Transaction
            </button>
          </div>
        </form>
      </div>
  
      <NotificationModal
        v-model="showModal"
        :title="modalTitle"
        :message="modalMessage"
      />
    </div>
</template>
  
<script>
  import { ref, onMounted, onBeforeUnmount } from 'vue';
  import ResVaultSDK from 'resvault-sdk';
  import NotificationModal from './NotificationModal.vue';
  import { useStore } from 'vuex';
  import { useRouter } from 'vue-router';
  
  export default {
    name: 'TransactionForm',
    components: {
      NotificationModal,
    },
    setup() {
      const store = useStore();
      const router = useRouter();
  
      const amount = ref('');
      const data = ref('');
      const recipient = ref('');
      const showModal = ref(false);
      const modalTitle = ref('');
      const modalMessage = ref('');
  
      const sdk = ref(null);
  
      if (!sdk.value) {
        sdk.value = new ResVaultSDK();
      }
  
      const handleLogout = () => {
        store.dispatch('logout');
      };
  
      const showErrorModal = (title, message) => {
        modalTitle.value = title;
        modalMessage.value = message;
        showModal.value = true;
      };
  
      const handleMessage = (event) => {
        const message = event.data;
  
        if (
          message &&
          message.type === 'FROM_CONTENT_SCRIPT' &&
          message.data &&
          message.data.success !== undefined
        ) {
          if (message.data.success) {
            modalTitle.value = 'Success';
            modalMessage.value =
              'Transaction successful! ID: ' + message.data.data.postTransaction.id;
          } else {
            modalTitle.value = 'Transaction Failed';
            modalMessage.value =
              'Transaction failed: ' +
              (message.data.error || JSON.stringify(message.data.errors));
          }
          showModal.value = true;
        }
      };
  
      const handleSubmit = () => {
        if (!recipient.value) {
          showErrorModal('Validation Error', 'Please enter a recipient address.');
          return;
        }
  
        let parsedData = {};
        if (data.value.trim() !== '') {
          try {
            parsedData = JSON.parse(data.value);
          } catch (error) {
            showErrorModal(
              'Validation Error',
              'Invalid JSON format in the data field. Please check and try again.'
            );
            return;
          }
        }
  
        if (sdk.value) {
          sdk.value.sendMessage({
            type: 'commit',
            direction: 'commit',
            amount: amount.value,
            data: parsedData,
            recipient: recipient.value,
          });
        } else {
          showErrorModal('Error', 'SDK is not initialized.');
        }
      };
  
      onMounted(() => {
        if (sdk.value) {
          sdk.value.addMessageListener(handleMessage);
        }
      });
  
      onBeforeUnmount(() => {
        if (sdk.value) {
          sdk.value.removeMessageListener(handleMessage);
        }
      });
  
      return {
        amount,
        data,
        recipient,
        showModal,
        modalTitle,
        modalMessage,
        handleSubmit,
        handleLogout,
      };
    },
  };
</script>