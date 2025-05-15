"use client"

import { useState } from 'react';
import styles from '../styles/installation.module.css';

const installCommands = {
  resilientdb: `
# Install ResilientDB
echo "Installing ResilientDB..."
git clone https://github.com/apache/incubator-resilientdb.git /opt/incubator-resilientdb
cd /opt/incubator-resilientdb
./INSTALL.sh
./service/tools/kv/server_tools/start_kv_service.sh
bazel build service/tools/kv/api_tools/kv_service_tools
`,
  pythonsdk: `
# Install PythonSDK
echo "Installing PythonSDK..."
git clone https://github.com/apache/incubator-resilientdb-python-sdk.git /opt/incubator-resilientdb-python-sdk
cd /opt/incubator-resilientdb-python-sdk
sh ./INSTALL.sh
python3.10 -m venv venv
source venv/bin/activate
pip install -r requirements.txt
bazel build service/http_server/crow_service_main
bazel-bin/service/http_server/crow_service_main service/tools/config/interface/client.config service/http_server/server_config.config
deactivate
`,
  resdborm: `
# Install ResDBORM
echo "Installing ResDBORM..."
git clone https://github.com/ResilientEcosystem/ResDB-ORM.git /opt/ResDB-ORM
cd /opt/ResDB-ORM
./INSTALL.sh
pip install resdb-orm
`,
  "smartcontracts-cli": `
# Install Smart-Contracts CLI
echo "Installing Smart-Contracts CLI..."
npm install -g rescontract-cli
`,
  "smartcontracts-graphql": `
# Install Smart-Contracts GraphQL
echo "Installing Smart-Contracts GraphQL..."
git clone https://github.com/ResilientEcosystem/smart-contracts-graphql.git /opt/smart-contracts-graphql
cd /opt/smart-contracts-graphql
npm install
`,
  resvault: `
# Install ResVault
echo "Installing ResVault..."
git clone https://github.com/apache/incubator-resilientdb-resvault.git /opt/ResVault
cd /opt/ResVault
npm install
npm run build
`
};

export function InstallationForm() {
  const [error, setError] = useState('');

  const handleGenerateScript = () => {
    const selectedApps = Array.from(document.querySelectorAll('input[name="app"]:checked')).map(cb => (cb as HTMLInputElement).value);
    if (selectedApps.length === 0) {
      setError("Please select at least one application to install.");
      return;
    }

    let scriptContent = "#!/bin/bash\n\n# Check for root privileges\nif [ \"$EUID\" -ne 0 ]; then\n  echo \"Please run as root. Try using 'sudo ./INSTALL.sh'\"\n  exit\nfi\n\n";
    scriptContent += "echo \"Updating package lists...\"\napt-get update\n";
    scriptContent += "echo \"Installing dependencies...\"\napt-get install -y git curl build-essential python3 python3-venv python3-pip npm solc\n\n";

    selectedApps.forEach(app => {
      scriptContent += installCommands[app as keyof typeof installCommands];
    });

    scriptContent += "\necho \"Installation complete. Please refer to the documentation for usage instructions.\"\n";

    const blob = new Blob([scriptContent], { type: "text/plain" });
    const link = document.getElementById("download-link");
    if (link) {
      link.setAttribute('href', URL.createObjectURL(blob));
      link.setAttribute('download', "INSTALL.sh");
      link.style.display = "block";
      link.textContent = "Download your custom INSTALL.sh";
    }
    setError('');
  };

  return (
    <div>
      <form id="install-form">
        <label><input type="checkbox" name="app" value="resilientdb" /> ResilientDB</label><br/>
        <label><input type="checkbox" name="app" value="pythonsdk" /> PythonSDK</label><br/>
        <label><input type="checkbox" name="app" value="resdborm" /> ResDBORM</label><br/>
        <label><input type="checkbox" name="app" value="smartcontracts-cli" /> Smart-Contracts CLI</label><br/>
        <label><input type="checkbox" name="app" value="smartcontracts-graphql" /> Smart-Contracts GraphQL</label><br/>
        <label><input type="checkbox" name="app" value="resvault" /> ResVault</label><br/>
        {error && <p style={{ color: 'red' }}>{error}</p>}
        <button type="button" className={styles.generateButton} onClick={handleGenerateScript}>
          Generate INSTALL.sh
        </button>
      </form>
      <a id="download-link" className={styles.downloadButton} style={{ display: 'none' }}>
        Download your custom INSTALL.sh
      </a>
    </div>
  );
} 