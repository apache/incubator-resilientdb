const { createGraphQLClient } = require('./graphql-client');

async function executeSmartContract(params) {
  const client = createGraphQLClient(params.apiUrl);

  const query = `
    mutation ExecuteSmartContract(
      $config: String!,
      $sender: String!,
      $contract: String!,
      $functionName: String!,
      $arguments: String!
    ) {
      executeContract(
        config: $config,
        sender: $sender,
        contract: $contract,
        functionName: $functionName,
        arguments: $arguments
      )
    }
  `;

  const variables = {
    config: params.config,
    sender: params.sender,
    contract: params.contractAddress,
    functionName: params.functionName,
    arguments: params.arguments.join(','),
  };

  try {
    const response = await client.request(query, variables);
    return response.executeContract;
  } catch (error) {
    throw new Error(`Error executing smart contract: ${error.message}`);
  }
}

module.exports = { executeSmartContract };
