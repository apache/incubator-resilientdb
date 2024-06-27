const fs = require('fs-extra');
const path = require('path');
const os = require('os');

const CONFIG_FILE_PATH = path.join(os.homedir(), '.smart-contracts-cli-config.json');

async function getResDBHome() {
  if (process.env.ResDB_Home) {
    return process.env.ResDB_Home;
  }

  if (await fs.pathExists(CONFIG_FILE_PATH)) {
    const config = await fs.readJson(CONFIG_FILE_PATH);
    if (config.resDBHome) {
      process.env.ResDB_Home = config.resDBHome;
      return config.resDBHome;
    }
  }

  return null;
}

async function setResDBHome(resDBHome) {
  process.env.ResDB_Home = resDBHome;
  await fs.writeJson(CONFIG_FILE_PATH, { resDBHome });
}

module.exports = {
  getResDBHome,
  setResDBHome
};
