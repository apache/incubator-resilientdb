const fs = require('fs-extra');
const path = require('path');
const os = require('os');

const CONFIG_FILE_PATH = path.join(os.homedir(), '.smart-contracts-cli-config.json');

async function getResDBHome() {
  try {
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
  } catch (error) {
    console.error(`Error accessing config file: ${error.message}`);
    return null;
  }
}

async function setResDBHome(resDBHome) {
  try {
    process.env.ResDB_Home = resDBHome;
    await fs.writeJson(CONFIG_FILE_PATH, { resDBHome });
  } catch (error) {
    console.error(`Error writing to config file: ${error.message}`);
  }
}

module.exports = {
  getResDBHome,
  setResDBHome
};
