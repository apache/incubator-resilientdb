/**
 * @typedef {Object} Env
 * @property {string} [PORT] - The port the application runs on.
 * @property {string} [PYROSCOPE_BASE_URL] - The base URL for the Pyroscope service.
 * @property {string} [NODE_EXPORTER_BASE_URL] - The base URL for the Node exporter service.
 * @property {string} [PROCESS_EXPORTER_BASE_URL] - The base URL for the Process exporter service.
 */

/**
 * Retrieves an environment variable by name, ensuring it's definsed if marked as mandatory.
 *
 * @param {keyof Env} name - The name of the environment variable to retrieve.
 * @param {any} [optional] - Default Value if env does not exist. throws an error if env is not defined and optional is not passed
 * @returns {string|undefined} The value of the environment variable if defined.
 * @throws {Error} If the environment variable is required and not defined.
 */
function getEnv(name, optional = null) {
    const value = process.env[name] || optional;
    
    if (value === undefined || value === null || value === "") {
      throw new Error(`Environment variable "${name}" is required but not defined.`);
    }
  
    return value;
  };


module.exports = {
    getEnv
}