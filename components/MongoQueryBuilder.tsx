'use client';

import React, { useState } from 'react';
import { Box, Button, Code, Paper, Select, Text, TextInput } from '@mantine/core';

const SAMPLE_DATA = {
  id: 'block_123',
  createdAt: '2024-04-27T10:00:00Z',
  transactions: [
    {
      id: 'tx_456',
      inputs: ['input1', 'input2'],
      outputs: ['output1'],
      metadata: { type: 'transfer' },
    },
  ],
};

export function MongoQueryBuilder() {
  const [field, setField] = useState('id');
  const [operator, setOperator] = useState('eq');
  const [value, setValue] = useState('');
  const [query, setQuery] = useState('');
  const [result, setResult] = useState('');

  const buildQuery = () => {
    let mongoQuery = '{\n';
    if (operator === 'eq') {
      mongoQuery += `  "${field}": "${value}"\n`;
    } else if (operator === 'gt' || operator === 'lt') {
      mongoQuery += `  "${field}": { "$${operator}": "${value}" }\n`;
    } else if (operator === 'in') {
      mongoQuery += `  "${field}": { "$in": ${value.split(',').map((v) => `"${v.trim()}"`)} }\n`;
    }
    mongoQuery += '}';

    setQuery(mongoQuery);
    setResult(JSON.stringify(SAMPLE_DATA, null, 2));
  };

  return (
    <Box>
      <Box mb="md">
        <Select
          label="Field"
          value={field}
          onChange={(val) => setField(val || 'id')}
          data={[
            { value: 'id', label: 'Block ID' },
            { value: 'createdAt', label: 'Creation Date' },
            { value: 'transactions.id', label: 'Transaction ID' },
            { value: 'transactions.metadata.type', label: 'Transaction Type' },
          ]}
        />
        <Select
          label="Operator"
          value={operator}
          onChange={(val) => setOperator(val || 'eq')}
          data={[
            { value: 'eq', label: 'Equals' },
            { value: 'gt', label: 'Greater Than' },
            { value: 'lt', label: 'Less Than' },
            { value: 'in', label: 'In Array' },
          ]}
        />
        <TextInput
          label="Value"
          value={value}
          onChange={(e) => setValue(e.target.value)}
          placeholder="Enter value..."
        />
      </Box>

      <Button onClick={buildQuery} mb="md">
        Build Query
      </Button>

      {query && (
        <>
          <Text fw={500} size="sm">
            Generated Query:
          </Text>
          <Paper withBorder p="xs" mb="md">
            <Code block>{query}</Code>
          </Paper>

          <Text fw={500} size="sm">
            Sample Result:
          </Text>
          <Paper withBorder p="xs">
            <Code block>{result}</Code>
          </Paper>
        </>
      )}
    </Box>
  );
}
