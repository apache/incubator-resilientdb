const bunyan = require("bunyan");
const { getEnv } = require("./envParser");

/**
 * Creates a Bunyan logger instance.
 */
const logger = bunyan.createLogger({
  name: "mem-lens-middleware", 
  level: getEnv("LOG_LEVEL","info"), 
  serializers: bunyan.stdSerializers,
})

module.exports = logger;
