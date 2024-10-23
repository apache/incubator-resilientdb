#!/usr/bin/env node
"use strict";
var __createBinding = (this && this.__createBinding) || (Object.create ? (function(o, m, k, k2) {
    if (k2 === undefined) k2 = k;
    var desc = Object.getOwnPropertyDescriptor(m, k);
    if (!desc || ("get" in desc ? !m.__esModule : desc.writable || desc.configurable)) {
      desc = { enumerable: true, get: function() { return m[k]; } };
    }
    Object.defineProperty(o, k2, desc);
}) : (function(o, m, k, k2) {
    if (k2 === undefined) k2 = k;
    o[k2] = m[k];
}));
var __setModuleDefault = (this && this.__setModuleDefault) || (Object.create ? (function(o, v) {
    Object.defineProperty(o, "default", { enumerable: true, value: v });
}) : function(o, v) {
    o["default"] = v;
});
var __importStar = (this && this.__importStar) || function (mod) {
    if (mod && mod.__esModule) return mod;
    var result = {};
    if (mod != null) for (var k in mod) if (k !== "default" && Object.prototype.hasOwnProperty.call(mod, k)) __createBinding(result, mod, k);
    __setModuleDefault(result, mod);
    return result;
};
var __awaiter = (this && this.__awaiter) || function (thisArg, _arguments, P, generator) {
    function adopt(value) { return value instanceof P ? value : new P(function (resolve) { resolve(value); }); }
    return new (P || (P = Promise))(function (resolve, reject) {
        function fulfilled(value) { try { step(generator.next(value)); } catch (e) { reject(e); } }
        function rejected(value) { try { step(generator["throw"](value)); } catch (e) { reject(e); } }
        function step(result) { result.done ? resolve(result.value) : adopt(result.value).then(fulfilled, rejected); }
        step((generator = generator.apply(thisArg, _arguments || [])).next());
    });
};
var __importDefault = (this && this.__importDefault) || function (mod) {
    return (mod && mod.__esModule) ? mod : { "default": mod };
};
Object.defineProperty(exports, "__esModule", { value: true });
const inquirer_1 = __importDefault(require("inquirer"));
const fs_extra_1 = __importDefault(require("fs-extra"));
const path = __importStar(require("path"));
const commander_1 = require("commander");
const program = new commander_1.Command();
program
    .version('1.0.0')
    .option('-n, --name <projectName>', 'Name of the project')
    .option('-f, --framework <framework>', 'Framework to use (react or vue)')
    .option('-l, --language <language>', 'Language to use (javascript or typescript)')
    .parse(process.argv);
const options = program.opts();
function promptUser() {
    return __awaiter(this, void 0, void 0, function* () {
        const questions = [];
        if (!options.name) {
            questions.push({
                type: 'input',
                name: 'projectName',
                message: 'What is the name of your project?',
                validate(input) {
                    if (!input) {
                        return 'Project name cannot be empty.';
                    }
                    if (fs_extra_1.default.existsSync(path.join(process.cwd(), input))) {
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
        const answers = yield inquirer_1.default.prompt(questions);
        return {
            projectName: options.name || answers.projectName,
            framework: options.framework || answers.framework,
            language: options.language || answers.language,
        };
    });
}
function getTemplatePath(framework, language) {
    const langSuffix = language.toLowerCase() === 'typescript' ? 'ts' : 'js';
    const templateDir = `${framework}-${langSuffix}-resvault-sdk`;
    let templatePath = path.resolve(__dirname, '..', 'templates', templateDir);
    if (!fs_extra_1.default.existsSync(templatePath)) {
        const alternativePath = path.resolve(__dirname, 'templates', templateDir);
        if (fs_extra_1.default.existsSync(alternativePath)) {
            templatePath = alternativePath;
        }
        else {
            throw new Error(`Template not found at ${templatePath} or ${alternativePath}`);
        }
    }
    return templatePath;
}
function copyTemplate(templatePath, targetPath) {
    return __awaiter(this, void 0, void 0, function* () {
        try {
            yield fs_extra_1.default.copy(templatePath, targetPath, { overwrite: true });
            console.log('Project setup complete!');
            console.log(`Navigate to your project by running: cd ${path.basename(targetPath)}`);
            console.log('Please run "npm install" to install dependencies.');
        }
        catch (err) {
            console.error('Error copying template:', err);
        }
    });
}
function main() {
    return __awaiter(this, void 0, void 0, function* () {
        try {
            const answers = yield promptUser();
            const { projectName, framework, language } = answers;
            console.log(`Selected Framework: ${framework}`);
            console.log(`Selected Language: ${language}`);
            console.log(`Project Name: ${projectName}`);
            const templatePath = getTemplatePath(framework, language);
            const targetPath = path.join(process.cwd(), projectName);
            if (fs_extra_1.default.existsSync(targetPath)) {
                console.error(`A directory named "${projectName}" already exists. Please choose a different project name.`);
                process.exit(1);
            }
            yield copyTemplate(templatePath, targetPath);
        }
        catch (error) {
            console.error('An error occurred:', error);
        }
    });
}
main();
