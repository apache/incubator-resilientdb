'use client';

import { useEffect, useState } from 'react';
import { python } from '@codemirror/lang-python';
import { IconPlayerPlay, IconTemplate, IconTrash } from '@tabler/icons-react';
import { vscodeDark } from '@uiw/codemirror-theme-vscode';
import CodeMirror from '@uiw/react-codemirror';
import { Box, Button, Group, Paper, Select, Stack, Text, Tooltip } from '@mantine/core';

declare global {
  interface Window {
    loadPyodide: (options: { indexURL: string }) => Promise<any>;
    pyodide: any;
  }
}

const EXAMPLE_TEMPLATES = [
  {
    value: 'default',
    label: 'Welcome - Start Here',
    code: `"""
Welcome to the ResilientDB Python Playground!

This interactive environment allows you to test and experiment with ResilientDB's key-value store 
using our Python SDK. The code here matches the patterns used in the official ResilientDB Python SDK.

Instructions:
1. Use the dropdown menu above to load example code
2. Modify the code as needed
3. Click 'Run' to execute the code
4. View results in the output panel below

Available Examples:
- Send Transaction: Create and send a new key-value transaction
- Get Transaction: Retrieve an existing transaction by ID
- Complete Workflow: Full example of sending and retrieving a transaction

Note: Each transaction needs a unique ID. Our examples use timestamps to generate these IDs.
"""`,
  },
  {
    value: 'send_transaction',
    label: 'Send Transaction',
    code: `"""
Example: Send a new key-value transaction to ResilientDB
"""
from resdb_sdk import ResilientDB, Transaction
import json
import time

# Initialize ResilientDB client
client = ResilientDB('https://crow.resilientdb.com')

# Create unique ID using timestamp
unique_id = f"test_{int(time.time())}"

# Create transaction object
transaction = Transaction(id=unique_id, value="Hello from ResilientDB!")

print(f"Creating transaction with ID: {unique_id}")
print("Transaction data:")
print(json.dumps(transaction.to_dict(), indent=2))

print("\\nSending transaction...")
result = await client.transactions.create(transaction)
print("Response:")
print(json.dumps(result, indent=2))`,
  },
  {
    value: 'get_transaction',
    label: 'Get Transaction',
    code: `"""
Example: Retrieve a transaction from ResilientDB
"""
from resdb_sdk import ResilientDB
import json

# Initialize ResilientDB client
client = ResilientDB('https://crow.resilientdb.com')

# Transaction ID to retrieve
# Replace this with an ID from a previously sent transaction
tx_id = "test_1234567890"

print(f"Retrieving transaction with ID: {tx_id}")
print("\\nSending GET request...")

result = await client.transactions.retrieve(tx_id)
print("\\nResponse:")
print(json.dumps(result, indent=2))`,
  },
  {
    value: 'complete_workflow',
    label: 'Complete Workflow (Send + Get)',
    code: `"""
Example: Complete workflow to send a transaction and then retrieve it
"""
from resdb_sdk import ResilientDB, Transaction
import json
import time

# Initialize ResilientDB client
client = ResilientDB('https://crow.resilientdb.com')

# Step 1: Create and send transaction
print("Step 1: Creating and sending transaction...")

# Generate unique ID using timestamp
unique_id = f"test_{int(time.time())}"

# Create transaction object
transaction = Transaction(id=unique_id, value="Hello from ResilientDB!")

print(f"Transaction ID: {unique_id}")
print("Transaction data:")
print(json.dumps(transaction.to_dict(), indent=2))

# Send transaction
send_result = await client.transactions.create(transaction)
print("\\nPOST Response:")
print(json.dumps(send_result, indent=2))

# Step 2: Retrieve the transaction
print("\\nStep 2: Retrieving the transaction...")
get_result = await client.transactions.retrieve(unique_id)
print("GET Response:")
print(json.dumps(get_result, indent=2))

print("\\nWorkflow complete!")`,
  },
];

export function PythonPlayground() {
  const [code, setCode] = useState(EXAMPLE_TEMPLATES[0].code);
  const [output, setOutput] = useState<string[]>(['Initializing Python environment...']);
  const [isLoading, setIsLoading] = useState(true);
  const [isRunning, setIsRunning] = useState(false);

  useEffect(() => {
    const initPython = async () => {
      try {
        if (window.pyodide) {
          setOutput(['✅ Python environment ready']);
          setIsLoading(false);
          return;
        }

        // Load Pyodide
        const script = document.createElement('script');
        script.src = 'https://cdn.jsdelivr.net/pyodide/v0.24.1/full/pyodide.js';
        document.head.appendChild(script);

        await new Promise((resolve) => {
          script.onload = resolve;
        });

        // Initialize Pyodide
        const pyodide = await window.loadPyodide({
          indexURL: 'https://cdn.jsdelivr.net/pyodide/v0.24.1/full/',
        });

        // Set up stdout/stderr capture
        await pyodide.runPythonAsync(`
          import sys
          from io import StringIO
          sys.stdout = StringIO()
          sys.stderr = StringIO()
        `);

        // Load and register our SDK
        const sdk = await fetch('/resdb_sdk.py').then((res) => res.text());
        await pyodide.runPythonAsync(sdk);

        // Register the SDK as a module
        await pyodide.runPythonAsync(`
import sys
import types
resdb_sdk = types.ModuleType('resdb_sdk')
resdb_sdk.__dict__.update(globals())
sys.modules['resdb_sdk'] = resdb_sdk
        `);

        window.pyodide = pyodide;
        setOutput(['✅ Python environment ready']);
        setIsLoading(false);
      } catch (error) {
        setOutput((prev) => [
          ...prev,
          `❌ Error: ${error instanceof Error ? error.message : 'Unknown error'}`,
        ]);
        setIsLoading(false);
      }
    };

    initPython();
  }, []);

  const runCode = async () => {
    if (isLoading || isRunning) {
      return;
    }
    setIsRunning(true);
    setOutput(['▶ Running code...']);

    try {
      // Wrap code in async function if it uses await
      const codeToRun = /\bawait\b/.test(code)
        ? `
async def __run():
${code
  .split('\n')
  .map((line) => `    ${line}`)
  .join('\n')}

import asyncio
loop = asyncio.get_event_loop()
loop.run_until_complete(__run())
`
        : code;

      // Run the code and capture output
      await window.pyodide.runPythonAsync(codeToRun);

      // Get output
      const stdout = await window.pyodide.runPythonAsync('sys.stdout.getvalue()');
      if (stdout) {
        setOutput((prev) => [...prev, stdout]);
      }

      setOutput((prev) => [...prev, '✅ Code execution completed']);
    } catch (error: any) {
      setOutput((prev) => [...prev, `❌ Error: ${error.message}`]);
    } finally {
      // Clear buffers
      await window.pyodide.runPythonAsync('sys.stdout.truncate(0)\nsys.stdout.seek(0)');
      setIsRunning(false);
    }
  };

  const clearOutput = () => setOutput([]);

  return (
    <Paper withBorder p="md" radius="md">
      <Stack>
        <Group justify="flex-end" align="center">
          <Tooltip label="Load example">
            <Select
              size="xs"
              placeholder="Load example"
              data={EXAMPLE_TEMPLATES}
              onChange={(value) =>
                value && setCode(EXAMPLE_TEMPLATES.find((t) => t.value === value)?.code || '')
              }
              leftSection={<IconTemplate size={14} />}
            />
          </Tooltip>
        </Group>

        <Box>
          <CodeMirror
            value={code}
            height="400px"
            theme={vscodeDark}
            extensions={[python()]}
            onChange={setCode}
            editable={!isLoading}
            basicSetup={{
              lineNumbers: true,
              highlightActiveLineGutter: true,
              highlightSpecialChars: true,
              history: true,
              foldGutter: true,
              drawSelection: true,
              dropCursor: true,
              allowMultipleSelections: true,
              indentOnInput: true,
              syntaxHighlighting: true,
              bracketMatching: true,
              closeBrackets: true,
              autocompletion: true,
              rectangularSelection: true,
              crosshairCursor: true,
              highlightActiveLine: true,
              highlightSelectionMatches: true,
              closeBracketsKeymap: true,
              defaultKeymap: true,
              searchKeymap: true,
              historyKeymap: true,
              foldKeymap: true,
              completionKeymap: true,
              lintKeymap: true,
            }}
          />
        </Box>

        <Group justify="space-between">
          <Button
            onClick={runCode}
            loading={isRunning}
            disabled={isLoading}
            color="blue"
            leftSection={<IconPlayerPlay size={14} />}
          >
            {isRunning ? 'Running...' : 'Run'}
          </Button>
          <Button
            onClick={clearOutput}
            variant="light"
            disabled={isLoading || output.length === 0}
            leftSection={<IconTrash size={14} />}
          >
            Clear Output
          </Button>
        </Group>

        <Paper
          withBorder
          p="xs"
          style={{
            backgroundColor: '#1e1e1e',
            color: '#33ff00',
            fontFamily: 'monospace',
            height: '200px',
            overflow: 'auto',
          }}
        >
          {output.map((line, i) => (
            <Text key={i} style={{ whiteSpace: 'pre-wrap', color: 'inherit' }}>
              {line}
            </Text>
          ))}
        </Paper>
      </Stack>
    </Paper>
  );
}
