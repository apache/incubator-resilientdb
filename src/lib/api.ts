import axios from 'axios';

const middlewareApi = axios.create({
    baseURL: import.meta.env.VITE_MIDDLEWARE_BASE_URL,
});

export {
    middlewareApi, //generic middleware api
}
