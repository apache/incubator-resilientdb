import { BASE_URL } from '@/static/url';
import axios from 'axios';

const middlewareApi = axios.create({
    baseURL: BASE_URL,
});

export {
    middlewareApi, //generic middleware api
}
