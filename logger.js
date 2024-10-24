import fs from 'fs-extra';
import { createLogger, format, transports } from 'winston';
import path from 'path';
import os from 'os';

// Define the log directory
const logDir = path.join(os.homedir(), '.smart-contracts-graphql-logs');
fs.ensureDirSync(logDir);

// Create the logger with Winston
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

export default logger;
