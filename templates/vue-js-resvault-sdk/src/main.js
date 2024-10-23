import { createApp } from 'vue';
import App from './App.vue';
import router from './router';
import store from './store';

import 'bootstrap/dist/css/bootstrap.min.css';
import 'bootstrap';

import BootstrapVue3 from 'bootstrap-vue-3';
import 'bootstrap-vue-3/dist/bootstrap-vue-3.css';

import './App.css';

const app = createApp(App);

app.use(store);
app.use(router);
app.use(BootstrapVue3);

app.mount('#app');

store.dispatch('initializeAuth');