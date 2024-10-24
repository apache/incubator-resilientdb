const axios = require('axios');
const { expect } = require('chai');

describe('GraphQL Server Reachability Test', function () {
  this.timeout(5000); // Set timeout to 5 seconds for the test

  it('should return status 200 for the GraphQL server', async function () {
    try {
      const response = await axios.post('http://localhost:8400/graphql', {
        query: `
          {
            __schema {
              types {
                name
              }
            }
          }
        `
      });
      // Check if the server responds with status 200
      expect(response.status).to.equal(200);
    } catch (error) {
      // If there's an error, fail the test
      throw new Error(`GraphQL server is not reachable: ${error.message}`);
    }
  });
});
