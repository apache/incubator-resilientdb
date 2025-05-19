"use client"

import { useEffect, useState } from "react"
import CodeMirror from "@uiw/react-codemirror"
import { python } from "@codemirror/lang-python"
import { vscodeDark } from "@uiw/codemirror-theme-vscode"
import { Box, Button, Group, Paper, Stack, Text, Select, Tooltip } from "@mantine/core"
import { IconPlayerPlay, IconTrash, IconTemplate } from "@tabler/icons-react"

declare global {
  interface Window {
    loadPyodide: (options: { indexURL: string }) => Promise<any>
    pyodide: any
  }
}

const EXAMPLE_TEMPLATES = [
  {
    value: "test_new_transaction",
    label: "Test New Transaction",
    code: `"""
Example: Post a new transaction with unique ID
"""
from resdb_sdk import Resdb
import json
import time

# Initialize ResilientDB client
db = Resdb('https://crow.resilientdb.com')

# Create transaction with unique ID but same format
unique_id = f"test_{int(time.time())}"
transaction = {
    "id": unique_id,
    "value": "Hello from Postman"  # Using same value format that worked
}

print(f"Using unique ID: {unique_id}")
print("Transaction data:")
print(json.dumps(transaction, indent=2))

print("\\nSending transaction...")
result = await db.transactions.send_commit(transaction)
print("Response from send_commit:")
print(json.dumps(result, indent=2))

print("\\nVerifying if transaction exists...")
get_result = await db.transactions.retrieve(unique_id)
print("GET Response:")
print(json.dumps(get_result, indent=2))`
  },
  {
    value: "get_transaction",
    label: "Get Transaction Only",
    code: `"""
Example: Get a transaction from ResilientDB
"""
from resdb_sdk import Resdb
import json

# Initialize ResilientDB client
db = Resdb('https://crow.resilientdb.com')

# Get transaction
tx_id = "testkey123"  # This ID exists and works
print(f"Getting transaction with ID: {tx_id}")
result = await db.transactions.retrieve(tx_id)
print("Response:")
print(json.dumps(result, indent=2))`
  },
  {
    value: "complete_workflow",
    label: "Complete Workflow (Post & Get)",
    code: `"""
Example: Complete workflow to post a transaction and then retrieve it
"""
from resdb_sdk import Resdb
import json
import time
import asyncio

# Initialize ResilientDB client
db = Resdb('https://crow.resilientdb.com')

# Create transaction
tx_id = f"test_{int(time.time())}"
transaction = {
    "id": tx_id,
    "value": "Hello from Python!"
}

print(f"Step 1: Created transaction with ID: {tx_id}")
print(f"Transaction data: {json.dumps(transaction, indent=2)}")

print("Step 2: Sending transaction...")
result = await db.transactions.send_commit(transaction)
print(f"Send result: {json.dumps(result, indent=2)}")

print("Step 3: Retrieving the transaction...")
# Try multiple times with delays
max_attempts = 3
for attempt in range(max_attempts):
    print(f"Get attempt {attempt + 1}/{max_attempts}...")
    get_result = await db.transactions.retrieve(tx_id)
    if get_result and get_result != "{}":
        print(f"Success! Transaction retrieved: {json.dumps(get_result, indent=2)}")
        break
    if attempt < max_attempts - 1:
        print("No data yet, waiting 2 seconds...")
        await asyncio.sleep(2)

print("Workflow complete!")`
  }
]

export function PythonPlayground() {
  const [code, setCode] = useState(EXAMPLE_TEMPLATES[0].code)
  const [output, setOutput] = useState<string[]>(["Initializing Python environment..."])
  const [isLoading, setIsLoading] = useState(true)
  const [isRunning, setIsRunning] = useState(false)

  useEffect(() => {
    const initPython = async () => {
      try {
        if (window.pyodide) {
          setOutput(["✅ Python environment ready"])
          setIsLoading(false)
          return
        }

        // Load Pyodide
        const script = document.createElement("script")
        script.src = "https://cdn.jsdelivr.net/pyodide/v0.24.1/full/pyodide.js"
        document.head.appendChild(script)

        await new Promise((resolve) => script.onload = resolve)
        
        // Initialize Pyodide
        const pyodide = await window.loadPyodide({
          indexURL: "https://cdn.jsdelivr.net/pyodide/v0.24.1/full/"
        })

        // Set up stdout/stderr capture
        await pyodide.runPythonAsync(`
          import sys
          from io import StringIO
          sys.stdout = StringIO()
          sys.stderr = StringIO()
        `)

        // Load and register our SDK
        const sdk = await fetch("/resdb_sdk.py").then(res => res.text())
        await pyodide.runPythonAsync(sdk)
        
        // Register the SDK as a module
        await pyodide.runPythonAsync(`
import sys
import types
resdb_sdk = types.ModuleType('resdb_sdk')
resdb_sdk.__dict__.update(globals())
sys.modules['resdb_sdk'] = resdb_sdk
        `)
        
        window.pyodide = pyodide
        setOutput(["✅ Python environment ready"])
        setIsLoading(false)
      } catch (error) {
        console.error("Failed to initialize Python:", error)
        setOutput(prev => [...prev, `❌ Error: ${error instanceof Error ? error.message : "Unknown error"}`])
        setIsLoading(false)
      }
    }

    initPython()
  }, [])

  const runCode = async () => {
    if (isLoading || isRunning) return
    setIsRunning(true)
    setOutput(["▶ Running code..."])

    try {
      // Wrap code in async function if it uses await
      const codeToRun = /\bawait\b/.test(code)
        ? `
async def __run():
${code.split('\n').map(line => '    ' + line).join('\n')}

import asyncio
loop = asyncio.get_event_loop()
loop.run_until_complete(__run())
`
        : code

      // Run the code and capture output
      await window.pyodide.runPythonAsync(codeToRun)
      
      // Get output
      const stdout = await window.pyodide.runPythonAsync("sys.stdout.getvalue()")
      if (stdout) setOutput(prev => [...prev, stdout])
      
      setOutput(prev => [...prev, "✅ Code execution completed"])
    } catch (error: any) {
      console.error("Python execution error:", error)
      setOutput(prev => [...prev, `❌ Error: ${error.message}`])
    } finally {
      // Clear buffers
      await window.pyodide.runPythonAsync("sys.stdout.truncate(0)\nsys.stdout.seek(0)")
      setIsRunning(false)
    }
  }

  const clearOutput = () => setOutput([])

  return (
    <Paper withBorder p="md" radius="md">
      <Stack>
        <Group justify="flex-end" align="center">
          <Tooltip label="Load example">
            <Select
              size="xs"
              placeholder="Load example"
              data={EXAMPLE_TEMPLATES}
              onChange={(value) => value && setCode(EXAMPLE_TEMPLATES.find(t => t.value === value)?.code || '')}
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
            {isRunning ? "Running..." : "Run"}
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
            backgroundColor: "#1e1e1e",
            color: "#33ff00",
            fontFamily: "monospace",
            height: "200px",
            overflow: "auto"
          }}
        >
          {output.map((line, i) => (
            <Text key={i} style={{ whiteSpace: "pre-wrap", color: "inherit" }}>
              {line}
            </Text>
          ))}
        </Paper>
      </Stack>
    </Paper>
  )
}