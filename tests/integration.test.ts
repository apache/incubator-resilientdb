const { executeSmartContract } = require('../src/index');

describe('Integration Test', () => {
  it('should execute a smart contract using the real API', async () => {
    const result = await executeSmartContract({
      apiUrl: 'http://localhost:8400/graphql',
      config: 'config-data',
      sender: '0xSenderAddress',
      contractAddress: '0xContractAddress',
      functionName: 'transfer',
      arguments: ['arg1', 'arg2'],
    });

    expect(result).toBeDefined();
  });
});
