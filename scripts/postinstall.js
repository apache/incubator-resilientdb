// scripts/postinstall.js
const { exec } = require('child_process');
const inquirer = require('inquirer');

async function installSolc() {
  const { confirmInstall } = await inquirer.prompt([
    {
      type: 'confirm',
      name: 'confirmInstall',
      message: 'solc is required to compile smart contracts. Do you want to install solc now? (Requires sudo permissions)',
      default: false,
    },
  ]);

  if (confirmInstall) {
    const commands = [
      'sudo add-apt-repository ppa:ethereum/ethereum',
      'sudo apt-get update',
      'sudo apt-get install -y solc',
    ];

    const install = (cmd, callback) => {
      exec(cmd, (error, stdout, stderr) => {
        if (error) {
          console.error(`Error: ${error.message}`);
          return callback(error);
        }
        if (stderr) {
          console.error(`stderr: ${stderr}`);
        }
        console.log(stdout);
        callback(null);
      });
    };

    for (const cmd of commands) {
      await new Promise((resolve, reject) => {
        install(cmd, (err) => {
          if (err) return reject(err);
          resolve();
        });
      });
    }
  } else {
    console.log('Please install solc manually to use the compile feature.');
  }
}

installSolc();
