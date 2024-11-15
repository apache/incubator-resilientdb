#!/usr/bin/env node

const { Command } = require('commander');
const { execFile } = require('child_process');
const path = require('path');
const { getResDBHome } = require('./config');
const logger = require('./logger');
const fs = require('fs');

const program = new Command();

async function ensureResDBHome() {
  try {
    const resDBHome = await getResDBHome();
    if (!resDBHome) {
      console.error('Error: ResDB_Home is not set. Please set the ResDB_Home environment variable or provide a config.yaml file.');
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

function handleExecFile(command, args, options = {}) {
  return new Promise((resolve, reject) => {
    const child = execFile(command, args, options);

    child.stdout.on('data', (data) => {
      process.stdout.write(data);
    });

    child.stderr.on('data', (data) => {
      process.stderr.write(data);
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
        'contract',
        'api_tools',
        'contract_tools'
      );

      if (!fs.existsSync(configPath)) {
        logger.error(`Config file not found at ${configPath}`);
        console.error(`Error: Config file not found at ${configPath}`);
        process.exit(1);
      }

      await handleExecFile(commandPath, ['create', '-c', configPath]);
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

      const command = 'solc';
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
      const resDBHome = await ensureResDBHome();
      const commandPath = path.join(
        resDBHome,
        'bazel-bin',
        'service',
        'tools',
        'contract',
        'api_tools',
        'contract_tools'
      );

      const argList = args.split(',').map((arg) => arg.trim());

      await handleExecFile(commandPath, [
        'deploy',
        '-c',
        configPath,
        '-p',
        contract,
        '-n',
        name,
        '-a',
        argList.join(','),
        '-m',
        owner,
      ]);
    } catch (error) {
      logger.error(`Error executing deploy command: ${error.message}`);
      console.error(`Error: ${error.message}`);
      process.exit(1);
    }
  });

program
  .command('execute')
  .description('Execute a smart contract function')
  .requiredOption('-c, --config <configPath>', 'Client configuration path')
  .requiredOption('-m, --sender <senderAddress>', 'Sender address')
  .requiredOption('-s, --contract <contractAddress>', 'Contract address')
  .requiredOption(
    '-f, --function-name <functionName>',
    'Function name with signature'
  )
  .requiredOption(
    '-a, --arguments <parameters>',
    'Function arguments (comma-separated)'
  )
  .action(async (options) => {
    try {
      const {
        config: configPath,
        sender,
        contract,
        functionName,
        arguments: args,
      } = options;
      const resDBHome = await ensureResDBHome();
      const commandPath = path.join(
        resDBHome,
        'bazel-bin',
        'service',
        'tools',
        'contract',
        'api_tools',
        'contract_tools'
      );

      const argList = args.split(',').map((arg) => arg.trim());

      await handleExecFile(commandPath, [
        'execute',
        '-c',
        configPath,
        '-m',
        sender,
        '-s',
        contract,
        '-f',
        functionName,
        '-a',
        argList.join(','),
      ]);
    } catch (error) {
      logger.error(`Error executing execute command: ${error.message}`);
      console.error(`Error: ${error.message}`);
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
        'contract',
        'api_tools',
        'contract_tools'
      );

      if (!fs.existsSync(configPath)) {
        logger.error(`Config file not found at ${configPath}`);
        console.error(`Error: Config file not found at ${configPath}`);
        process.exit(1);
      }

      await handleExecFile(commandPath, [
        'add_address',
        '-c',
        configPath,
        '-e',
        externalAddress,
      ]);
    } catch (error) {
      logger.error(`Error executing add_address command: ${error.message}`);
      console.error(`Error: ${error.message}`);
      process.exit(1);
    }
  });

if (!process.argv.slice(2).length) {
  program.outputHelp();
}

program.parse(process.argv);