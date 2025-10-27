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
    <div v-if="modalVisible" class="modal-backdrop" :class="{ show: modalVisible }"></div>
  
    <div
      v-if="modalVisible"
      class="modal custom-modal"
      tabindex="-1"
      aria-labelledby="exampleModalLabel"
      aria-hidden="true"
      ref="modal"
      :style="{ display: modalVisible ? 'block' : 'none' }"
      @click.self="handleClose"
    >
      <div class="modal-dialog modal-dialog-centered">
        <div class="modal-content">
          <div class="modal-header">
            <h5 class="modal-title">{{ title }}</h5>
            <button type="button" class="btn-close" @click="handleClose" aria-label="Close"></button>
          </div>
  
          <div class="modal-body">
            <p>{{ message }}</p>
          </div>
  
          <div class="modal-footer">
            <button type="button" class="btn btn-primary" @click="handleClose">OK</button>
          </div>
        </div>
      </div>
    </div>
  </template>
  
<script lang="ts">
  import { defineComponent, computed } from 'vue';
  
  export default defineComponent({
    name: 'NotificationModal',
    props: {
      modelValue: {
        type: Boolean,
        required: true,
      },
      title: {
        type: String,
        required: true,
      },
      message: {
        type: String,
        required: true,
      },
    },
    emits: ['update:modelValue'],
    setup(props, { emit }) {
      const modalVisible = computed({
        get: () => props.modelValue,
        set: (value) => emit('update:modelValue', value),
      });
  
      const handleClose = () => {
        modalVisible.value = false;
      };
  
      return {
        modalVisible,
        handleClose,
      };
    },
  });
</script>
  
<style>
  .modal-backdrop {
    position: fixed;
    top: 0;
    left: 0;
    width: 100vw;
    height: 100vh;
    background-color: rgba(0, 0, 0, 0.5);
    z-index: 1040;
    opacity: 0;
    transition: opacity 0.3s ease;
  }
  
  .modal-backdrop.show {
    opacity: 0.5;
  }
</style>  