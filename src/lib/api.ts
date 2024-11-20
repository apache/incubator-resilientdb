import axios from 'axios';

const pyroscopeApi = axios.create({
    baseURL: import.meta.env.VITE_PYROSCOPE_BASE_URL,
});

export {
    pyroscopeApi
}
