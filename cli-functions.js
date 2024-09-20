const { execFile } = require('child_process');
const path = require('path');
const logger = require('./logger');

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

async function createAccount(configPath) {
  const command = 'rescontract'; 
  const args = ['create', '--config', configPath];
  return handleExecFile(command, args);
}

async function compileContract(sourcePath, outputPath) {
  const command = 'rescontract';
  const args = ['compile', '--sol', sourcePath, '--output', outputPath];
  return handleExecFile(command, args);
}

async function deployContract(config, contract, name, args, owner) {
  const command = 'rescontract';
  const argList = args.split(',').map((arg) => arg.trim());
  const cliArgs = [
    'deploy',
    '--config',
    config,
    '--contract',
    contract,
    '--name',
    name,
    '--arguments',
    argList.join(','),
    '--owner',
    owner,
  ];
  return handleExecFile(command, cliArgs);
}

async function executeContract(config, sender, contract, functionName, args) {
  const command = 'rescontract';
  const argList = args.split(',').map((arg) => arg.trim());
  const cliArgs = [
    'execute',
    '--config',
    config,
    '--sender',
    sender,
    '--contract',
    contract,
    '--function-name',
    functionName,
    '--arguments',
    argList.join(','),
  ];
  return handleExecFile(command, cliArgs);
}

module.exports = {
  createAccount,
  compileContract,
  deployContract,
  executeContract,
};

