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

import inquirer from 'inquirer';
import fs from 'fs-extra';
import * as path from 'path';
import { Command } from 'commander';

interface Answers {
  projectName: string;
  framework: 'react' | 'vue';
  language: 'typescript' | 'javascript';
}

const program = new Command();

program
  .version('1.0.0')
  .option('-n, --name <projectName>', 'Name of the project')
  .option('-f, --framework <framework>', 'Framework to use (react or vue)')
  .option('-l, --language <language>', 'Language to use (javascript or typescript)')
  .parse(process.argv);

const options = program.opts();

async function promptUser() {
  const questions: inquirer.DistinctQuestion<Answers>[] = [];

  if (!options.name) {
    questions.push({
      type: 'input',
      name: 'projectName',
      message: 'What is the name of your project?',
      validate(input: string) {
        if (!input) {
          return 'Project name cannot be empty.';
        }
        if (fs.existsSync(path.join(process.cwd(), input))) {
          return 'A directory with this name already exists.';
        }
        return true;
      },
    });
  }

  if (!options.framework) {
    questions.push({
      type: 'list',
      name: 'framework',
      message: 'Which framework would you like to use?',
      choices: ['react', 'vue'],
    });
  }

  if (!options.language) {
    questions.push({
      type: 'list',
      name: 'language',
      message: 'Which language would you like to use?',
      choices: ['typescript', 'javascript'],
    });
  }

  const answers = await inquirer.prompt<Answers>(questions);

  return {
    projectName: options.name || answers.projectName,
    framework: options.framework || answers.framework,
    language: options.language || answers.language,
  };
}

function getTemplatePath(framework: string, language: string): string {
  const langSuffix = language.toLowerCase() === 'typescript' ? 'ts' : 'js';
  const templateDir = `${framework}-${langSuffix}-resvault-sdk`;
  let templatePath = path.resolve(__dirname, '..', 'templates', templateDir);

  if (!fs.existsSync(templatePath)) {
    const alternativePath = path.resolve(__dirname, 'templates', templateDir);
    if (fs.existsSync(alternativePath)) {
      templatePath = alternativePath;
    } else {
      throw new Error(`Template not found at ${templatePath} or ${alternativePath}`);
    }
  }

  return templatePath;
}

async function copyTemplate(templatePath: string, targetPath: string) {
  try {
    await fs.copy(templatePath, targetPath, { overwrite: true });
    console.log('Project setup complete!');
    console.log(`Navigate to your project by running: cd ${path.basename(targetPath)}`);
    console.log('Please run "npm install" to install dependencies.');
  } catch (err) {
    console.error('Error copying template:', err);
  }
}

async function main() {
  try {
    const answers = await promptUser();
    const { projectName, framework, language } = answers;

    console.log(`Selected Framework: ${framework}`);
    console.log(`Selected Language: ${language}`);
    console.log(`Project Name: ${projectName}`);

    const templatePath = getTemplatePath(framework, language);
    const targetPath = path.join(process.cwd(), projectName);

    if (fs.existsSync(targetPath)) {
      console.error(`A directory named "${projectName}" already exists. Please choose a different project name.`);
      process.exit(1);
    }

    await copyTemplate(templatePath, targetPath);
  } catch (error) {
    console.error('An error occurred:', error);
  }
}

main();