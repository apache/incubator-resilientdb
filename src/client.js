import axios from 'axios';

export const sendRequest = async (query) => {
    const headers = {
        'Content-Type': 'application/json'
    };
    const data = {
      query: query
    }
    const response = await axios.post(process.env.REACT_APP_GRAPHQL_SERVER, data, { headers })
    return response.data;
};
