#!/usr/bin/env node

const { Command } = require('commander');
const { exec } = require('child_process');
const path = require('path');
const inquirer = require('inquirer');
const { getResDBHome, setResDBHome } = require('./config');
const fs = require('fs');

const program = new Command();
const logFilePath = path.join(__dirname, 'cli-logs.log');

async function promptForResDBHome() {
  const answers = await inquirer.prompt([
    {
      type: 'input',
      name: 'resDBHome',
      message: 'Please enter the ResDB_Home path:',
    },
  ]);

  const resDBHome = answers.resDBHome;
  await setResDBHome(resDBHome);

  return resDBHome;
}

function logMessage(level, message) {
  const timestamp = new Date().toISOString();
  const logEntry = `${timestamp} ${level}: ${message}\n`;
  fs.appendFileSync(logFilePath, logEntry);
}

function handleExec(command) {
  exec(command, (error, stdout, stderr) => {
    if (error) {
      console.error(`Error: ${error.message}`);
      logMessage('error', `Command: ${command}\n${error.message}`);
      return;
    }
    if (stderr) {
      console.error(`stderr: ${stderr}`);
      logMessage('warn', `Command: ${command}\nstderr: ${stderr}`);
      return;
    }
    console.log(`stdout: ${stdout}`);
    logMessage('info', `Command: ${command}\nstdout: ${stdout}`);
  });
}

program
  .version('1.0.0')
  .description('Smart Contracts CLI');

program
  .command('create')
  .description('Create a new account')
  .option('-c, --config <path>', 'Path to the config file')
  .action(async (options) => {
    const configPath = options.config;
    if (!configPath) {
      console.error('Error: Config file path is required');
      logMessage('error', 'Config file path is required');
      process.exit(1);
    }

    let resDBHome = await getResDBHome();
    if (!resDBHome) {
      resDBHome = await promptForResDBHome();
    }

    const command = `${path.join(resDBHome, 'bazel-bin/service/tools/contract/api_tools/contract_tools')} create -c ${configPath}`;
    handleExec(command);
  });

program
  .command('compile')
  .description('Compile a .sol file to a .json file')
  .option('-s, --sol <path>', 'Path to the .sol file')
  .option('-o, --output <name>', 'Name of the output .json file')
  .action(async (options) => {
    const solPath = options.sol;
    const outputName = options.output;
    if (!solPath || !outputName) {
      console.error('Error: .sol file path and output name are required');
      logMessage('error', '.sol file path and output name are required');
      process.exit(1);
    }

    let resDBHome = await getResDBHome();
    if (!resDBHome) {
      resDBHome = await promptForResDBHome();
    }

    const command = `solc --evm-version homestead --combined-json bin,hashes --pretty-json --optimize ${solPath} > ${outputName}`;
    handleExec(command);
  });

program
  .command('deploy')
  .description('Deploy a smart contract')
  .option('-c, --config <configPath>', 'Client configuration path')
  .option('-p, --contract <contractPath>', 'Contract path')
  .option('-n, --name <contractName>', 'Contract name')
  .option('-a, --arguments <parameters>', 'Parameters to construct the contract object')
  .option('-m, --owner <ownerAddress>', 'Contract owner’s address')
  .action(async (options) => {
    const { config: configPath, contract, name, arguments: args, owner } = options;
    if (!configPath || !contract || !name || !args || !owner) {
      console.error('Error: All options (-c, -p, -n, -a, -m) are required.');
      logMessage('error', 'All options (-c, -p, -n, -a, -m) are required.');
      process.exit(1);
    }

    let resDBHome = await getResDBHome();
    if (!resDBHome) {
      resDBHome = await promptForResDBHome();
    }

    const command = `${path.join(resDBHome, 'bazel-bin/service/tools/contract/api_tools/contract_tools')} deploy -c ${configPath} -p ${contract} -n ${name} -a ${args} -m ${owner}`;
    handleExec(command);
  });

program
  .command('execute')
  .description('Execute a smart contract function')
  .option('-c, --config <configPath>', 'Client configuration path')
  .option('-m, --sender <senderAddress>', 'Sender address')
  .option('-s, --contract <contractAddress>', 'Contract address')
  .option('-f, --function <functionName>', 'Function name with signature')
  .option('-a, --arguments <parameters>', 'Arguments for the function')
  .action(async (options) => {
    const { config: configPath, sender, contract, function: functionName, arguments: args } = options;
    if (!configPath || !sender || !contract || !functionName || !args) {
      console.error('Error: All options (-c, -m, -s, -f, -a) are required.');
      logMessage('error', 'All options (-c, -m, -s, -f, -a) are required.');
      process.exit(1);
    }

    let resDBHome = await getResDBHome();
    if (!resDBHome) {
      resDBHome = await promptForResDBHome();
    }

    const command = `${path.join(resDBHome, 'bazel-bin/service/tools/contract/api_tools/contract_tools')} execute -c ${configPath} -m ${sender} -s ${contract} -f "${functionName}" -a ${args}`;
    handleExec(command);
  });

program.parse(process.argv);

