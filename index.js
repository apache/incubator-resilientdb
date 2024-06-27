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

program.parse(process.argv);
