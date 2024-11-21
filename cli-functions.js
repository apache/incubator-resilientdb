import fs from 'fs';
import os from 'os';
import crypto from 'crypto';
import { execFile } from 'child_process';
import path from 'path';
import logger from './logger.js';

const compiledContractsDir = path.join(new URL('.', import.meta.url).pathname, 'compiled_contracts');

if (!fs.existsSync(compiledContractsDir)) {
  fs.mkdirSync(compiledContractsDir, { recursive: true });
}

function handleExecFile(command, args, options = {}) {
  return new Promise((resolve, reject) => {
    const child = execFile(command, args, options);

    let stdoutData = '';
    let stderrData = '';

    child.stdout.on('data', (data) => {
      stdoutData += data;
    });

    child.stderr.on('data', (data) => {
      stderrData += data;
    });

    child.on('error', (error) => {
      logger.error(`Error spawning child process: ${error.message}`);
      reject(`Error spawning child process: ${error.message}`);
    });

    child.on('close', (code) => {
      if (code !== 0) {
        const errorMessage = `Process exited with code ${code}\n${stderrData}`;
        logger.error(errorMessage);
        reject(errorMessage);
      } else {
        const output = (stdoutData + stderrData).trim();
        resolve(output);
      }
    });
  });
}

export async function createAccount(config, type = 'path') {
  let configPath = config;

  if (type === 'data') {
    const filename = path.join(os.tmpdir(), `config-${crypto.randomUUID()}.tmp`);
    fs.writeFileSync(filename, config);
    configPath = filename;
  }

  const command = 'rescontract';
  const args = ['create', '--config', configPath];

  try {
    const result = await handleExecFile(command, args);
    if (type === 'data') {
      fs.unlinkSync(configPath);
    }
    const address = result.match(/address: \"(0x[0-9a-fA-F]+)\"/)[1];
    return address;
  } catch (error) {
    if (type === 'data' && fs.existsSync(configPath)) {
      fs.unlinkSync(configPath);
    }
    throw error;
  }
}

export async function addAddress(config, address, type = 'path') {
  let configPath = config;

  if (type === 'data') {
    const configFilename = path.join(os.tmpdir(), `config-${crypto.randomUUID()}.tmp`);
    fs.writeFileSync(configFilename, config);
    configPath = configFilename;
  }

  const command = 'rescontract';
  const cliArgs = ['add_address', '-c', configPath, '-e', address];

  try {
    const result = await handleExecFile(command, cliArgs);

    if (result.includes("Address added successfully")) {
      return "Address added successfully";
    } else {
      throw new Error("Failed to add address. Unexpected output: " + result);
    }
  } catch (error) {
    throw error;
  } finally {
    if (type === 'data' && fs.existsSync(configPath)) {
      fs.unlinkSync(configPath);
    }
  }
}


export async function compileContract(source, type = 'path') {
  let sourcePath = source;
  let outputPath = '';

  if (type === 'data') {
    const uniqueId = crypto.randomUUID();
    const sourceFilename = path.join(os.tmpdir(), `contract-${uniqueId}.sol`);
    outputPath = path.join(compiledContractsDir, `contract-${uniqueId}.json`);
    fs.writeFileSync(sourceFilename, source);
    sourcePath = sourceFilename;
  } else {
    outputPath = path.join(compiledContractsDir, 'MyContract.json'); 
  }

  const command = 'rescontract';
  const args = ['compile', '--sol', sourcePath, '--output', outputPath];

  try {
    const result = await handleExecFile(command, args);
    if (type === 'data') {
      return path.basename(outputPath);
    }
    const successMessage = result.match(/Compiled successfully to (\/[^\s]+)/)[0];
    return successMessage;
  } catch (error) {
    throw error;
  } finally {
    if (type === 'data') {
      if (fs.existsSync(sourcePath)) fs.unlinkSync(sourcePath);
    }
  }
}

export async function deployContract(config, contract, name, args, owner, type = 'path') {
  let configPath = config;
  let contractPath = contract;

  if (type === 'data') {
    const configFilename = path.join(os.tmpdir(), `config-${crypto.randomUUID()}.tmp`);
    fs.writeFileSync(configFilename, config);
    configPath = configFilename;
    contractPath = path.join(compiledContractsDir, contract);
  }

  const command = 'rescontract';

  const argList = args.split(',').map((arg) => arg.trim());
  const cliArgs = [
    'deploy',
    '--config',
    configPath,
    '--contract',
    contractPath,
    '--name',
    name,
    '--arguments',
    argList.join(','),
    '--owner',
    owner,
  ];

  try {
    const result = await handleExecFile(command, cliArgs);
    const ownerAddress = result.match(/owner_address: \"(0x[0-9a-fA-F]+)\"/)[1];
    const contractAddress = result.match(/contract_address: \"(0x[0-9a-fA-F]+)\"/)[1];
    const contractName = result.match(/contract_name: \"([^\"]+)\"/)[1];

    return {
      ownerAddress,
      contractAddress,
      contractName
    };
  } catch (error) {
    throw error;
  } finally {
    if (type === 'data' && fs.existsSync(configPath)) {
      fs.unlinkSync(configPath);
    }
  }
}

export async function executeContract(config, sender, contract, functionName, args, type = 'path') {
  let configPath = config;

  if (type === 'data') {
    const configFilename = path.join(os.tmpdir(), `config-${crypto.randomUUID()}.tmp`);
    fs.writeFileSync(configFilename, config);
    configPath = configFilename;
  }

  const command = 'rescontract';

  const argList = args.split(',').map((arg) => arg.trim());
  const cliArgs = [
    'execute',
    '--config',
    configPath,
    '--sender',
    sender,
    '--contract',
    contract,
    '--function-name',
    functionName,
    '--arguments',
    argList.join(','),
  ];

  try {
    const result = await handleExecFile(command, cliArgs);
    const success = result.includes("0x0000000000000000000000000000000000000000000000000000000000000001");
    return success ? "Execution successful" : "Execution failed";
    return result;
  } catch (error) {
    throw error;
  } finally {
    if (type === 'data' && fs.existsSync(configPath)) {
      fs.unlinkSync(configPath);
    }
  }
}
