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
