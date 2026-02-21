#!/usr/bin/env node
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

const { Command } = require('commander');
const { execFile } = require('child_process');
const path = require('path');
const fs = require('fs');
const os = require('os');
const { getResDBHome } = require('./config');
const logger = require('./logger');
const { spawn } = require('child_process');

const program = new Command();

async function ensureResDBHome() {
  try {
    const resDBHome = await getResDBHome();
    if (!resDBHome) {
      console.error(
        'Error: ResDB_Home is not set. Please set the ResDB_Home environment variable or provide a config.yaml file.'
      );
      logger.error('ResDB_Home is not set.');
      process.exit(1);
    }
    return resDBHome;
  } catch (error) {
    console.error(`Error: ${error.message}`);
    logger.error(`Error: ${error.message}`);
    process.exit(1);
  }
}

// Define the path for the deployed contracts registry file
const registryFilePath = path.join(
  os.homedir(),
  '.rescontract_deployed_contracts.json'
);

// Function to load the deployed contracts registry from file
function loadDeployedContracts() {
  if (fs.existsSync(registryFilePath)) {
    const data = fs.readFileSync(registryFilePath, 'utf-8');
    return new Map(JSON.parse(data));
  }
  return new Map();
}

// Function to save the deployed contracts registry to file
function saveDeployedContracts(registry) {
  const data = JSON.stringify(Array.from(registry.entries()), null, 2);
  fs.writeFileSync(registryFilePath, data, 'utf-8');
}

// Load the registry at the start
const deployedContracts = loadDeployedContracts();

function handleExecFile(command, args, options = {}, onData) {
  return new Promise((resolve, reject) => {
    const child = execFile(command, args, options);

    child.stdout.on('data', (data) => {
      process.stdout.write(data);
      if (onData) onData(data);
    });

    child.stderr.on('data', (data) => {
      process.stderr.write(data);
      if (onData) onData(data);
    });

    child.on('error', (error) => {
      logger.error(`Error spawning child process: ${error.message}`);
      reject(error);
    });

    child.on('exit', (code) => {
      if (code !== 0) {
        logger.error(`Process exited with code ${code}`);
        reject(new Error(`Process exited with code ${code}`));
      } else {
        resolve();
      }
    });
  });
}

// Function to handle the process execution
function handleSpawnProcess(command, args, options = {}, onData) {
  return new Promise((resolve, reject) => {
    const child = spawn(command, args, options);

    let output = '';

    child.stdout.on('data', (data) => {
      process.stdout.write(data); // Print to CLI
      output += data.toString(); // Accumulate output
      if (onData) onData(data.toString());
    });

    child.stderr.on('data', (data) => {
      process.stderr.write(data); // Print errors to CLI
      output += data.toString(); // Accumulate output
      if (onData) onData(data.toString());
    });

    child.on('error', (error) => {
      reject(error); // Handle spawn error
    });

    child.on('close', (code) => {
      if (code !== 0) {
        reject(new Error(`Process exited with code ${code}`));
      } else {
        resolve(output); // Return the full output
      }
    });
  });
}

program
  .name('rescontract')
  .version('1.2.2')
  .description('ResContract CLI - Manage smart contracts in ResilientDB');

program
  .command('create')
  .description('Create a new account')
  .requiredOption('-c, --config <path>', 'Path to the config file')
  .action(async (options) => {
    try {
      const configPath = options.config;
      const resDBHome = await ensureResDBHome();
      const commandPath = path.join(
        resDBHome,
        'bazel-bin',
        'service',
        'tools',
        'kv',
        'api_tools',
        'contract_service_tools'
      );

      if (!fs.existsSync(configPath)) {
        logger.error(`Config file not found at ${configPath}`);
        console.error(`Error: Config file not found at ${configPath}`);
        process.exit(1);
      }

      // Create JSON command file
      const jsonConfig = { command: 'create_account' };
      const jsonConfigPath = path.join(os.tmpdir(), `create_account_${Date.now()}.json`);
      fs.writeFileSync(jsonConfigPath, JSON.stringify(jsonConfig, null, 2));

      let output = '';
      try {
        output = await handleSpawnProcess(commandPath, ['-c', configPath, '--config_file', jsonConfigPath]);
      } finally {
        // Clean up temporary file
        if (fs.existsSync(jsonConfigPath)) {
          fs.unlinkSync(jsonConfigPath);
        }
      }

      // Parse address from output (C++ tool prints to stderr as address: "0x...")
      const match = output.match(/"?address"?\s*:\s*"(0x[0-9a-fA-F]+)"/);
      const address = match ? match[1] : '';
      if (!address) {
        console.error(
          JSON.stringify({
            error: 'Failed to parse account address from output.',
          })
        );
        process.exit(1);
      }
      console.log(JSON.stringify({ address }));
    } catch (error) {
      logger.error(`Error executing create command: ${error.message}`);
      console.error(`Error: ${error.message}`);
      process.exit(1);
    }
  });

program
  .command('compile')
  .description('Compile a .sol file to a .json file')
  .requiredOption('-s, --sol <path>', 'Path to the .sol file')
  .requiredOption('-o, --output <name>', 'Name of the output .json file')
  .action(async (options) => {
    try {
      const solPath = options.sol;
      const outputName = options.output;

      if (!fs.existsSync(solPath)) {
        logger.error(`Solidity file not found at ${solPath}`);
        console.error(`Error: Solidity file not found at ${solPath}`);
        process.exit(1);
      }

      const command = process.env.SOLC_PATH || 'solc';

      const args = [
        '--evm-version',
        'homestead',
        '--combined-json',
        'bin,hashes',
        '--pretty-json',
        '--optimize',
        solPath,
      ];

      await new Promise((resolve, reject) => {
        const child = execFile(command, args, (error, stdout, stderr) => {
          if (error) {
            logger.error(`Compilation failed: ${error.message}`);
            console.error(`Error: Compilation failed: ${error.message}`);
            reject(error);
            return;
          }

          fs.writeFileSync(outputName, stdout);
          logger.info(`Successfully compiled ${solPath} to ${outputName}`);
          console.log(`Compiled successfully to ${outputName}`);
          resolve();
        });
      });
    } catch (error) {
      logger.error(`Error executing compile command: ${error.message}`);
      console.error(`Error: ${error.message}`);
      process.exit(1);
    }
  });

program
  .command('deploy')
  .description('Deploy a smart contract')
  .requiredOption('-c, --config <configPath>', 'Client configuration path')
  .requiredOption('-p, --contract <contractPath>', 'Contract JSON file path')
  .requiredOption('-n, --name <contractName>', 'Contract name')
  .requiredOption(
    '-a, --arguments <parameters>',
    'Constructor parameters (comma-separated)'
  )
  .requiredOption('-m, --owner <ownerAddress>', 'Contract owner’s address')
  .action(async (options) => {
    try {
      const {
        config: configPath,
        contract,
        name,
        arguments: args,
        owner,
      } = options;

      const deploymentKey = `${owner}:${name}`;

      if (deployedContracts.has(deploymentKey)) {
        const existingContractAddress =
          deployedContracts.get(deploymentKey).contractAddress;
        console.error(
          JSON.stringify({
            error: `Contract "${name}" is already deployed by owner "${owner}" at address "${existingContractAddress}".`,
          })
        );
        process.exit(1);
      }

      const resDBHome = await ensureResDBHome();
      const commandPath = path.join(
        resDBHome,
        'bazel-bin',
        'service',
        'tools',
        'kv',
        'api_tools',
        'contract_service_tools'
      );
      // Create JSON command file
      const jsonConfig = {
        command: 'deploy',
        contract_path: contract,
        contract_name: name,
        init_params: args,
        owner_address: owner
      };
      const jsonConfigPath = path.join(os.tmpdir(), `deploy_${Date.now()}.json`);
      fs.writeFileSync(jsonConfigPath, JSON.stringify(jsonConfig, null, 2));

      let output;
      try {
        output = await handleSpawnProcess(commandPath, ['-c', configPath, '--config_file', jsonConfigPath]);
      } finally {
        // Clean up temporary file
        if (fs.existsSync(jsonConfigPath)) {
          fs.unlinkSync(jsonConfigPath);
        }
      }
               
      const outputLines = output.split('\n');
      let ownerAddress = '';
      let contractAddress = '';
      let contractName = '';

      for (const line of outputLines) {
        const content = line.replace(/^.*\] /, '').trim();

        const ownerMatch = content.match(/owner_address:\s*"(.+)"$/);
        const contractAddressMatch = content.match(/contract_address:\s*"(.+)"$/);
        const contractNameMatch = content.match(/contract_name:\s*"(.+)"$/);

        if (ownerMatch) {
          ownerAddress = ownerMatch[1];
        } else if (contractAddressMatch) {
          contractAddress = contractAddressMatch[1];
        } else if (contractNameMatch) {
          contractName = contractNameMatch[1];
        }
      }

      if (!ownerAddress || !contractAddress || !contractName) {
        console.error(
          JSON.stringify({
            error: 'Failed to parse deployment output.',
          })
        );
        process.exit(1);
      }

      deployedContracts.set(deploymentKey, {
        ownerAddress: ownerAddress,
        contractAddress: contractAddress,
        contractName: contractName,
      });

      saveDeployedContracts(deployedContracts);

      console.log(
        JSON.stringify({
          owner_address: ownerAddress,
          contract_address: contractAddress,
          contract_name: contractName,
        })
      );
    } catch (error) {
      console.error(
        JSON.stringify({
          error: error.message,
        })
      );
      process.exit(1);
    }
  });

program
  .command('add_address')
  .description('Add an external address to the system')
  .requiredOption('-c, --config <path>', 'Path to the config file')
  .requiredOption('-e, --external-address <address>', 'External address to add')
  .action(async (options) => {
    try {
      const configPath = options.config;
      const externalAddress = options.externalAddress;
      const resDBHome = await ensureResDBHome();
      const commandPath = path.join(
        resDBHome,
        'bazel-bin',
        'service',
        'tools',
        'kv',
        'api_tools',
        'contract_service_tools'
      );

      if (!fs.existsSync(configPath)) {
        logger.error(`Config file not found at ${configPath}`);
        console.error(`Error: Config file not found at ${configPath}`);
        process.exit(1);
      }

      const jsonConfig = {
        command: 'add_address',
        address: externalAddress
      };
      const jsonConfigPath = path.join(os.tmpdir(), `add_address_${Date.now()}.json`);
      fs.writeFileSync(jsonConfigPath, JSON.stringify(jsonConfig, null, 2));
      
      try {
        await handleExecFile(commandPath, ['-c', configPath, '--config_file', jsonConfigPath]);
      } finally {
        // Clean up temporary file
        if (fs.existsSync(jsonConfigPath)) {
          fs.unlinkSync(jsonConfigPath);
        }
      }
    } catch (error) {
      logger.error(`Error executing add_address command: ${error.message}`);
      console.error(`Error: ${error.message}`);
      process.exit(1);
    }
  });

// Optional commands to manage the registry

program
  .command('list-deployments')
  .description('List all deployed contracts')
  .action(() => {
    if (deployedContracts.size === 0) {
      console.log('No contracts have been deployed yet.');
    } else {
      console.log('Deployed Contracts:');
      for (const [key, value] of deployedContracts.entries()) {
        console.log(`Owner: ${value.ownerAddress}`);
        console.log(`Contract Name: ${value.contractName}`);
        console.log(`Contract Address: ${value.contractAddress}`);
        console.log('---');
      }
    }
  });

program
  .command('clear-registry')
  .description('Clear the deployed contracts registry')
  .action(() => {
    try {
      if (fs.existsSync(registryFilePath)) {
        fs.unlinkSync(registryFilePath);
        deployedContracts.clear();
        console.log('Deployed contracts registry cleared.');
      } else {
        console.log('Deployed contracts registry is already empty.');
      }
    } catch (error) {
      logger.error(`Error clearing registry: ${error.message}`);
      console.error(`Error: ${error.message}`);
      process.exit(1);
    }
  });

if (!process.argv.slice(2).length) {
  program.outputHelp();
}

program.parse(process.argv);
