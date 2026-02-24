import axios from 'axios';

const createApiInstance = (baseURL: string) => {
    return axios.create({
        baseURL,
    });
};

const middlewareApi = createApiInstance(import.meta.env.VITE_MIDDLEWARE_BASE_URL);
const middlewareSecondaryApi = createApiInstance(import.meta.env.VITE_MIDDLEWARE_SECONDARY_BASE_URL);

console.log(import.meta.env.VITE_MIDDLEWARE_SECONDARY_BASE_URL);

export {
    middlewareApi, 
    middlewareSecondaryApi,
}
