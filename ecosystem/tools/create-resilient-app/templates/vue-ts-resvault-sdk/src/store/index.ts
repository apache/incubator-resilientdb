/**
* Licensed to the Apache Software Foundation (ASF) under one
* or more contributor license agreements.  See the NOTICE file
* distributed with this work for additional information
* regarding copyright ownership.  The ASF licenses this file
* to you under the Apache License, Version 2.0 (the
* "License"); you may not use this file except in compliance
* with the License.  You may obtain a copy of the License at
* 
*   http://www.apache.org/licenses/LICENSE-2.0
* 
* Unless required by applicable law or agreed to in writing,
* software distributed under the License is distributed on an
* "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
* KIND, either express or implied.  See the License for the
* specific language governing permissions and limitations
* under the License.    
*/

import { createStore } from 'vuex';

export default createStore({
  state: {
    isAuthenticated: false,
    token: null as string | null,
    isLoadingAfterLogin: false,
  },
  mutations: {
    setAuthentication(state, payload: { isAuthenticated: boolean; token: string | null }) {
      state.isAuthenticated = payload.isAuthenticated;
      state.token = payload.token;
    },
    setLoading(state, payload: boolean) {
      state.isLoadingAfterLogin = payload;
    },
  },
  actions: {
    login({ commit }, token: string) {
      commit('setLoading', true);
      sessionStorage.setItem('token', token);
      setTimeout(() => {
        commit('setAuthentication', { isAuthenticated: true, token });
        commit('setLoading', false);
      }, 2000);
    },
    logout({ commit }) {
      commit('setAuthentication', { isAuthenticated: false, token: null });
      commit('setLoading', false);
      sessionStorage.removeItem('token');
    },
    initializeAuth({ commit }) {
      const storedToken = sessionStorage.getItem('token');
      if (storedToken) {
        commit('setAuthentication', { isAuthenticated: true, token: storedToken });
      }
      commit('setLoading', false);
    },
    stopLoading({ commit }) {
      commit('setLoading', false);
    },
  },
  getters: {
    isAuthenticated: (state) => state.isAuthenticated,
    token: (state) => state.token,
    isLoadingAfterLogin: (state) => state.isLoadingAfterLogin,
  },
  modules: {},
});