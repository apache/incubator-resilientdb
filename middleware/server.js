require("dotenv").config();
const app = require("./app");
const logger = require("./utils/logger");
const { getEnv } = require('./utils/envParser');

const PORT = getEnv("PORT",3000);

app.listen(PORT, () => {
  logger.info(`Server is running at http://localhost:${PORT}`);
});
