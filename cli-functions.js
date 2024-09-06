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
  createAccount: (configPath) => handleExec(`create --config ${configPath}`),
  compileContract: (path) => handleExec(`compile --path ${path}`),
  deployContract: (path) => handleExec(`deploy --path ${path}`),
  executeContract: (command) => handleExec(`execute ${command}`)
};

