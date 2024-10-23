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