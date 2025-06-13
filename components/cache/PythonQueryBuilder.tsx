'use client';

import React, { useState } from 'react';
import { Box, Button, Code, Paper, Select, Text, TextInput } from '@mantine/core';

const SAMPLE_DATA = {
  id: "block_123",
  createdAt: new Date().toISOString(),
  transactions: [
    {
      id: "tx_123",
      inputs: [
        {
          owner: "user1",
          amount: 100
        }
      ],
      outputs: [
        {
          owner: "user2",
          amount: 100
        }
      ],
      metadata: {
        timestamp: 1746736411543,
        type: "transfer"
      },
      asset: {
        data: {
          type: "transfer",
          amount: 100,
          from: "user1",
          to: "user2"
        }
      }
    }
  ]
};

export function PythonQueryBuilder() {
  const [field, setField] = useState('id');
  const [operator, setOperator] = useState('eq');
  const [value, setValue] = useState('');
  const [query, setQuery] = useState('');
  const [result, setResult] = useState('');

  const buildQuery = () => {
    let mongoQuery = '{\n';
    if (operator === 'eq') {
      const numValue = Number(value);
      if (!isNaN(numValue) && (
        field === 'transactions.asset.data.amount' || 
        field === 'transactions.metadata.timestamp'
      )) {
        mongoQuery += `  "${field}": ${numValue}\n`;
      } else {
        mongoQuery += `  "${field}": "${value}"\n`;
      }
    } else if (operator === 'gt' || operator === 'lt') {
      const numValue = Number(value);
      if (!isNaN(numValue)) {
        mongoQuery += `  "${field}": { "$${operator}": ${numValue} }\n`;
      } else {
        mongoQuery += `  "${field}": { "$${operator}": "${value}" }\n`;
      }
    } else if (operator === 'in') {
      mongoQuery += `  "${field}": { "$in": ${value.split(',').map((v) => {
        const numValue = Number(v.trim());
        return !isNaN(numValue) ? numValue : `"${v.trim()}"`;
      })} }\n`;
    } else if (operator === 'regex') {
      mongoQuery += `  "${field}": { "$regex": "${value}", "$options": "i" }\n`;
    } else if (operator === 'exists') {
      mongoQuery += `  "${field}": { "$exists": ${value === 'true'} }\n`;
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
            { value: 'createdAt', label: 'Created At' },
            { value: 'transactions.id', label: 'Transaction ID' },
            { value: 'transactions.inputs.owner', label: 'Input Owner' },
            { value: 'transactions.outputs.owner', label: 'Output Owner' },
            { value: 'transactions.metadata.timestamp', label: 'Transaction Timestamp' },
            { value: 'transactions.metadata.type', label: 'Transaction Type' },
            { value: 'transactions.asset.data.type', label: 'Asset Type' },
            { value: 'transactions.asset.data.amount', label: 'Asset Amount' }
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
            { value: 'regex', label: 'Pattern Match' },
            { value: 'exists', label: 'Field Exists' }
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