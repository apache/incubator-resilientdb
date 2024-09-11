const { exec } = require('child_process');
const path = require('path');
const inquirer = require('inquirer');
const { getResDBHome, setResDBHome } = require('../ResContract/config');
const fs = require('fs');

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
  return new Promise((resolve, reject) => {
    exec(command, (error, stdout, stderr) => {
      if (error) {
        logMessage('error', `Command: ${command}\n${error.message}`);
        reject(`Error: ${error.message}`);
      } else if (stderr) {
        logMessage('warn', `Command: ${command}\nstderr: ${stderr}`);
        reject(`stderr: ${stderr}`);
      } else {
        logMessage('info', `Command: ${command}\nstdout: ${stdout}`);
        resolve(stdout);
      }
    });
  });
}

module.exports = {
  createAccount: (configPath) => handleExec(`smart-contracts-cli create --config ${configPath}`),
  compileContract: (sourcePath, outputPath) => handleExec(`smart-contracts-cli compile -s ${sourcePath} -o ${outputPath}`),
  deployContract: (config, contract, name, args, owner) => handleExec(`smart-contracts-cli deploy --config ${config} --contract ${contract} --name ${name} --arguments ${args} --owner ${owner}`),
  executeContract: (config, sender, contract, func, args) => handleExec(`smart-contracts-cli execute --config ${config} --sender ${sender} --contract ${contract} --function ${func} --arguments ${args}`)
};
