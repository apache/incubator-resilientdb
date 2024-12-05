const { GraphQLClient } = require('graphql-request');

function createGraphQLClient(apiUrl) {
  return new GraphQLClient(apiUrl, {
    headers: { 'Content-Type': 'application/json' },
  });
}

module.exports = { createGraphQLClient };
