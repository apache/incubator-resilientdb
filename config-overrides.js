// config-overrides.js
module.exports = function override(config, env) {
  if (env === 'production') {
    // Disable inline runtime chunk to prevent CSP violations
    config.optimization.runtimeChunk = false;

    // Ensure that chunk files have unique names
    config.output.filename = 'static/js/[name].js';
    config.output.chunkFilename = 'static/js/[name].chunk.js';
  }
  return config;
};
