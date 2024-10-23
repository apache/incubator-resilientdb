
# Create Resilient App

`create-resilient-app` is a CLI tool for scaffolding a new React or Vue application with ResVault SDK integration. It supports both JavaScript and TypeScript.

## Features

- Supports both **React** and **Vue** frameworks.
- Choose between **JavaScript** and **TypeScript**.
- Automatically sets up your project with the appropriate file structure, ResVault SDK, and dependencies.
- Easy to use with flags or interactive prompts.

## Installation

You don't need to install it globally. Just run the command with `npx`:

```bash
npx create-resilient-app
```

Alternatively, you can install it globally:

```bash
npm install -g create-resilient-app
```

## Usage

### Interactive Mode

Running `npx create-resilient-app` without flags will start an interactive prompt to help you set up your project.

```bash
npx create-resilient-app
```

You will be asked the following questions:

1. **Project Name**: The name of your project.
2. **Framework**: Choose between `React` or `Vue`.
3. **Language**: Choose between `JavaScript` or `TypeScript`.

### Using Flags

You can also bypass the interactive mode by providing all the necessary options through flags:

```bash
npx create-resilient-app --name my-app --framework react --language typescript
```

#### Flags

- `-n, --name`: Name of the project (required if not using interactive mode).
- `-f, --framework`: Choose the framework: `react` or `vue` (required if not using interactive mode).
- `-l, --language`: Choose the language: `javascript` or `typescript` (required if not using interactive mode).

### Example Commands

- **Create a React app using TypeScript**:

  ```bash
  npx create-resilient-app --name my-react-app --framework react --language typescript
  ```

- **Create a Vue app using JavaScript**:

  ```bash
  npx create-resilient-app --name my-vue-app --framework vue --language javascript
  ```

## Project Setup

Once the project is generated, navigate to your project directory:

```bash
cd my-app
```

Then, install the project dependencies:

```bash
npm install
```

### Running the Project

After installing the dependencies, you can run the project:

For **React**:

```bash
npm start
```

For **Vue**:

```bash
npm run dev
```

### Customizing the Project

The project includes a basic setup with ResVault SDK integrated. Feel free to customize the project as per your needs.

## Contributing

Feel free to open issues or submit pull requests if you find bugs or want to add new features.

## License

This project is licensed under the Apache-2.0 License.
