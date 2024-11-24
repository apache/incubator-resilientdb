import axios from 'axios';

const middlewareApi = axios.create({
    baseURL: import.meta.env.VITE_MIDDLEWARE_BASE_URL,
});


const pyroscopeApi = axios.create({
    baseURL: import.meta.env.VITE_PYROSCOPE_BASE_URL,
});

export {
    pyroscopeApi, //agent specific api not working due to CORS
    middlewareApi //generic middleware api
}
