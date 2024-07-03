#!/usr/bin/env node

const { Command } = require('commander');
const { exec } = require('child_process');
const path = require('path');
const inquirer = require('inquirer');
const { getResDBHome, setResDBHome } = require('./config');

const program = new Command();

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
      process.exit(1);
    }

    let resDBHome = await getResDBHome();
    if (!resDBHome) {
      resDBHome = await promptForResDBHome();
    }

    const command = `${path.join(resDBHome, 'bazel-bin/service/tools/contract/api_tools/contract_tools')} create -c ${configPath}`;

    exec(command, (error, stdout, stderr) => {
      if (error) {
        console.error(`Error: ${error.message}`);
        return;
      }
      if (stderr) {
        console.error(`stderr: ${stderr}`);
        return;
      }
      console.log(`stdout: ${stdout}`);
    });
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
      process.exit(1);
    }
    let resDBHome = await getResDBHome();
    if (!resDBHome) {
      resDBHome = await promptForResDBHome();
    }

    const command = `solc --evm-version homestead --combined-json bin,hashes --pretty-json --optimize ${solPath} > ${outputName}`;

    exec(command, (error, stdout, stderr) => {
      if (error) {
        console.error(`Error: ${error.message}`);
        return;
      }
      if (stderr) {
        console.error(`stderr: ${stderr}`);
        return;
      }
      console.log(`stdout: ${stdout}`);
    });
  });

program.parse(process.argv);
