const fs = require('fs-extra');
const { createLogger, format, transports } = require('winston');
const path = require('path');
const os = require('os');

const logDir = path.join(os.homedir(), '.smart-contracts-graphql-logs');
fs.ensureDirSync(logDir);

const logger = createLogger({
  level: 'info',
  format: format.combine(
    format.timestamp(),
    format.printf(({ level, message, timestamp }) => {
      return `${timestamp} ${level}: ${message}`;
    })
  ),
  transports: [
    new transports.File({ filename: path.join(logDir, 'cli.log') }),
    new transports.Console({ format: format.simple() })
  ],
  exceptionHandlers: [
    new transports.File({ filename: path.join(logDir, 'exceptions.log') })
  ]
});

module.exports = logger;

