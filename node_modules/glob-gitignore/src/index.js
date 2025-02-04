const make_array = require('make-array')
const vanilla = require('glob')

const {sync} = require('./sync')
const {glob} = require('./glob')

const hasMagic = (patterns, options) =>
  make_array(patterns)
  .some(pattern => vanilla.hasMagic(pattern, options))

module.exports = {
  sync,
  glob,
  hasMagic
}
