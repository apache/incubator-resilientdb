/*
* Licensed to the Apache Software Foundation (ASF) under one
* or more contributor license agreements.  See the NOTICE file
* distributed with this work for additional information
* regarding copyright ownership.  The ASF licenses this file
* to you under the Apache License, Version 2.0 (the
* "License"); you may not use this file except in compliance
* with the License.  You may obtain a copy of the License at
* 
*   http://www.apache.org/licenses/LICENSE-2.0
* 
* Unless required by applicable law or agreed to in writing,
* software distributed under the License is distributed on an
* "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
* KIND, either express or implied.  See the License for the
* specific language governing permissions and limitations
* under the License.
*/

'use client';

import { useState } from 'react';
import { Box, Tabs, TabsList, TabsTab, TabsPanel, Alert, Button, Paper, Text, Group, Code } from '@mantine/core';
import { IconInfoCircle, IconPlayerPlay, IconCheck, IconX, IconCopy } from '@tabler/icons-react';
import CodeMirror from '@uiw/react-codemirror';
import { javascript } from '@codemirror/lang-javascript';
import { vscodeDark } from '@uiw/codemirror-theme-vscode';

const DEFAULT_CONTRACT = `// SPDX-License-Identifier: MIT
pragma solidity ^0.8.0;

contract SimpleToken {
    string public name;
    string public symbol;
    uint256 public totalSupply;
    mapping(address => uint256) public balanceOf;

    constructor(string memory _name, string memory _symbol, uint256 _totalSupply) {
        name = _name;
        symbol = _symbol;
        totalSupply = _totalSupply;
        balanceOf[msg.sender] = _totalSupply;
    }

    function transfer(address _to, uint256 _value) public returns (bool) {
        require(balanceOf[msg.sender] >= _value, "Insufficient balance");
        balanceOf[msg.sender] -= _value;
        balanceOf[_to] += _value;
        return true;
    }
}`;

const EXAMPLE_CONTRACTS = {
  token: `// Basic ERC-20 Token Contract
// Try modifying the token name, symbol, or total supply`,
  voting: `// Voting Contract
// Experiment with different voting mechanisms`,
  auction: `// Auction Contract
// Try implementing different auction types`
};

const TUTORIAL_STEPS = {
  step1: `// Step 1: Setup
// Make sure you have Node.js and solc installed`,
  step2: `// Step 2: Create Contract
// Your first smart contract
// Follow the instructions in the comments`,
  step3: `// Step 3: Deploy
// We'll use a test network for this tutorial`,
  step4: `// Step 4: Interact
// Try calling different functions
// Experiment with different parameters`
};

interface CompilationResult {
  success: boolean;
  output: string;
  abi?: any;
  bytecode?: string;
}

export function ResContractPlayground() {
  const [code, setCode] = useState(DEFAULT_CONTRACT);
  const [activeTab, setActiveTab] = useState('playground');
  const [activeExample, setActiveExample] = useState('token');
  const [activeStep, setActiveStep] = useState('step1');
  const [compilationResult, setCompilationResult] = useState<CompilationResult | null>(null);
  const [isCompiling, setIsCompiling] = useState(false);

  const handleCompile = async () => {
    setIsCompiling(true);
    setCompilationResult(null);
    
    try {
      const response = await fetch('/api/compile', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ code }),
      });

      if (!response.ok) {
        throw new Error('Failed to compile contract');
      }

      const result = await response.json();
      setCompilationResult(result);
    } catch (error) {
      setCompilationResult({
        success: false,
        output: 'Failed to compile contract. Please check your code and try again.',
      });
    } finally {
      setIsCompiling(false);
    }
  };

  const copyToClipboard = (text: string) => {
    navigator.clipboard.writeText(text);
  };

  return (
    <Tabs defaultValue="playground" value={activeTab} onChange={(value) => setActiveTab(value as string)}>
      <TabsList>
        <TabsTab value="playground">Smart Contract Playground</TabsTab>
        <TabsTab value="examples">Example Contracts</TabsTab>
        <TabsTab value="tutorial">Step-by-Step Tutorial</TabsTab>
      </TabsList>

      <TabsPanel value="playground">
        <Box my="md">
          <Alert icon={<IconInfoCircle size={16} />} title="Interactive Playground" color="blue">
            Experiment with a simple token contract in our interactive playground. Modify the code and see the results instantly.
          </Alert>
          <Box mt="md">
            <CodeMirror
              value={code}
              height="400px"
              theme={vscodeDark}
              extensions={[javascript()]}
              onChange={(value) => setCode(value)}
            />
          </Box>
          <Group mt="md" justify="flex-end">
            <Button
              leftSection={<IconPlayerPlay size={16} />}
              onClick={handleCompile}
              loading={isCompiling}
            >
              Compile Contract
            </Button>
          </Group>
          {compilationResult && (
            <Paper mt="md" p="md" withBorder>
              <Group mb="xs">
                {compilationResult.success ? (
                  <IconCheck size={20} color="green" />
                ) : (
                  <IconX size={20} color="red" />
                )}
                <Text fw={500}>
                  {compilationResult.success ? 'Compilation Successful' : 'Compilation Failed'}
                </Text>
              </Group>
              <Text size="sm" style={{ whiteSpace: 'pre-wrap' }}>
                {compilationResult.output}
              </Text>
              {compilationResult.success && (
                <>
                  <Box mt="md">
                    <Group justify="space-between" mb="xs">
                      <Text size="sm" fw={500}>Contract ABI:</Text>
                      <Button
                        variant="subtle"
                        size="xs"
                        leftSection={<IconCopy size={14} />}
                        onClick={() => copyToClipboard(JSON.stringify(compilationResult.abi, null, 2))}
                      >
                        Copy
                      </Button>
                    </Group>
                    <CodeMirror
                      value={JSON.stringify(compilationResult.abi, null, 2)}
                      height="200px"
                      theme={vscodeDark}
                      extensions={[javascript()]}
                      readOnly
                    />
                  </Box>
                  <Box mt="md">
                    <Group justify="space-between" mb="xs">
                      <Text size="sm" fw={500}>Bytecode:</Text>
                      <Button
                        variant="subtle"
                        size="xs"
                        leftSection={<IconCopy size={14} />}
                        onClick={() => copyToClipboard(compilationResult.bytecode || '')}
                      >
                        Copy
                      </Button>
                    </Group>
                    <Code block>
                      {compilationResult.bytecode}
                    </Code>
                  </Box>
                </>
              )}
            </Paper>
          )}
        </Box>
      </TabsPanel>

      <TabsPanel value="examples">
        <Box my="md">
          <Alert icon={<IconInfoCircle size={16} />} title="Example Contracts" color="blue">
            Explore and learn from these example smart contracts. Each example demonstrates different concepts and patterns.
          </Alert>
          <Tabs defaultValue="token" value={activeExample} onChange={(value) => setActiveExample(value as string)}>
            <TabsList>
              <TabsTab value="token">Token Contract</TabsTab>
              <TabsTab value="voting">Voting Contract</TabsTab>
              <TabsTab value="auction">Auction Contract</TabsTab>
            </TabsList>

            <TabsPanel value="token">
              <Box mt="md">
                <CodeMirror
                  value={EXAMPLE_CONTRACTS.token}
                  height="200px"
                  theme={vscodeDark}
                  extensions={[javascript()]}
                  onChange={(value) => setCode(value)}
                />
              </Box>
            </TabsPanel>

            <TabsPanel value="voting">
              <Box mt="md">
                <CodeMirror
                  value={EXAMPLE_CONTRACTS.voting}
                  height="200px"
                  theme={vscodeDark}
                  extensions={[javascript()]}
                  onChange={(value) => setCode(value)}
                />
              </Box>
            </TabsPanel>

            <TabsPanel value="auction">
              <Box mt="md">
                <CodeMirror
                  value={EXAMPLE_CONTRACTS.auction}
                  height="200px"
                  theme={vscodeDark}
                  extensions={[javascript()]}
                  onChange={(value) => setCode(value)}
                />
              </Box>
            </TabsPanel>
          </Tabs>
        </Box>
      </TabsPanel>

      <TabsPanel value="tutorial">
        <Box my="md">
          <Alert icon={<IconInfoCircle size={16} />} title="Interactive Tutorial" color="blue">
            Follow this step-by-step tutorial to learn how to create, deploy, and interact with smart contracts.
          </Alert>
          <Tabs defaultValue="step1" value={activeStep} onChange={(value) => setActiveStep(value as string)}>
            <TabsList>
              <TabsTab value="step1">Step 1: Setup</TabsTab>
              <TabsTab value="step2">Step 2: Create Contract</TabsTab>
              <TabsTab value="step3">Step 3: Deploy</TabsTab>
              <TabsTab value="step4">Step 4: Interact</TabsTab>
            </TabsList>

            <TabsPanel value="step1">
              <Alert icon={<IconInfoCircle size={16} />} title="Prerequisites" color="blue" mb="md">
                Make sure you have Node.js and solc installed before proceeding.
              </Alert>
            </TabsPanel>

            <TabsPanel value="step2">
              <Box mt="md">
                <CodeMirror
                  value={TUTORIAL_STEPS.step2}
                  height="200px"
                  theme={vscodeDark}
                  extensions={[javascript()]}
                  onChange={(value) => setCode(value)}
                />
              </Box>
            </TabsPanel>

            <TabsPanel value="step3">
              <Alert icon={<IconInfoCircle size={16} />} title="Note" color="blue" mb="md">
                We'll use a test network for this tutorial.
              </Alert>
            </TabsPanel>

            <TabsPanel value="step4">
              <Box mt="md">
                <CodeMirror
                  value={TUTORIAL_STEPS.step4}
                  height="200px"
                  theme={vscodeDark}
                  extensions={[javascript()]}
                  onChange={(value) => setCode(value)}
                />
              </Box>
            </TabsPanel>
          </Tabs>
        </Box>
      </TabsPanel>
    </Tabs>
  );
} 