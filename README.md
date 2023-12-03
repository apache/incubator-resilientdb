# ResDB-CLI

`resdb-cli` is a command-line interface for managing ResDB instances and Python SDK instances. It provides functionalities to create, delete, view, and manage instances in a simple and efficient way.

## Table of Contents

- [Installation](#installation)
- [Configuration](#configuration)
- [Usage](#usage)
- [Commands](#commands)
- [Contributing](#contributing)
- [License](#license)

## Installation

To use `resdb-cli`, you can download the binary from the [Releases](https://github.com/your-username/resdb-cli/releases) page on GitHub.

1. Go to the [Releases](https://github.com/gopuman/resdb-cli/releases) page.
2. Download the latest release for your operating system (e.g., `resdb-cli-linux` for Linux).
3. Make the downloaded binary executable:

    ```bash
    chmod +x resdb-cli
    ```

## Configuration

`resdb-cli` uses a configuration file (`config.ini`) to store settings such as the MongoDB URI. Follow the steps below to configure the CLI:

Create a configuration file named `config.ini`, Replace the MongoDB_URI value with the appropriate MongoDB connection string.:

   ```ini
    [Database]
    MongoDB_URI = mongodb://localhost:27017/
    name = resui

    [User]
    current_user = gn@gmail.com
   ```

## Usage

Once installed and configured, you can use resdb-cli to perform various actions related to ResDB and Python SDK instances. Run the CLI with the following command:

```bash
    ./resdb-cli
```

## Commands

- Login: Logs in to the specified user account.

```bash
    ./resdb-cli login # Enter email and password when prompted
```

- Logout: Logs out from the current user account.

```bash
    ./resdb-cli logout
```

- Create Instance: Creates a new ResDB or Python SDK instance.

```bash
    ./resdb-cli create_instance <type>
```

- View Instances: Displays details about running instances.

```bash
    ./resdb-cli view_instances
```

- Delete Instance: Deletes a running ResDB or Python SDK instance.

```bash
    ./resdb-cli delete_instance <instance_id>
```

- Current User: Displays the currently logged-in user.

```bash
    ./resdb-cli whoami
```

For more detailed information about each command, run ./resdb-cli --help or ./resdb-cli <command> --help.