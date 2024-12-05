const { executeSmartContract } = require('../src/index');

jest.mock('../src/graphql-client', () => {
  return {
    createGraphQLClient: jest.fn().mockReturnValue({
      request: jest.fn().mockResolvedValue({ executeContract: 'Success' }),
    }),
  };
});

describe('executeSmartContract', () => {
  it('should execute a smart contract and return success', async () => {
    const result = await executeSmartContract({
      apiUrl: 'http://localhost:8400/graphql',
      config: 'config-data',
      sender: '0xSenderAddress',
      contractAddress: '0xContractAddress',
      functionName: 'transfer',
      arguments: ['arg1', 'arg2'],
    });

    expect(result).toBe('Success');
  });
});
