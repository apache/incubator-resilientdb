const {GlobSync} = require('glob')
const inherits = require('util.inherits')
const {
  IGNORE,
  createTasks
} = require('./util')

function _GlobSync (pattern, options, shouldIgnore) {
  this[IGNORE] = shouldIgnore
  GlobSync.call(this, pattern, options)
}

inherits(_GlobSync, GlobSync)

_GlobSync.prototype._readdir = function _readdir (abs, inGlobStar) {
  // `options.nodir` makes `options.mark` as `true`.
  // Mark `abs` first
  // to make sure `'node_modules'` will be ignored immediately
  //   with ignore pattern `'node_modules/'`.

  // There is a built-in cache about marked `File.Stat` in `glob`,
  //   so that we could not worry about the extra invocation of `this._mark()`
  const marked = this._mark(abs)

  if (this[IGNORE] && this[IGNORE](marked)) {
    return null
  }

  return GlobSync.prototype._readdir.call(this, abs, inGlobStar)
}

exports.sync = (_patterns, options = {}) => {
  const {
    join,
    patterns,
    ignores,
    opts,
    result
  } = createTasks(_patterns, options)

  if (result) {
    return result
  }

  const groups = patterns.map(
    pattern => new _GlobSync(pattern, opts, ignores).found
  )

  return join(groups)
}
