[![Build Status](https://travis-ci.org/kaelzhang/node-util-inherits.svg?branch=master)](https://travis-ci.org/kaelzhang/node-util-inherits)
<!-- optional appveyor tst
[![Windows Build Status](https://ci.appveyor.com/api/projects/status/github/kaelzhang/node-util-inherits?branch=master&svg=true)](https://ci.appveyor.com/project/kaelzhang/node-util-inherits)
-->
<!-- optional npm version
[![NPM version](https://badge.fury.io/js/util-inherits.svg)](http://badge.fury.io/js/util-inherits)
-->
<!-- optional npm downloads
[![npm module downloads per month](http://img.shields.io/npm/dm/util-inherits.svg)](https://www.npmjs.org/package/util-inherits)
-->
<!-- optional dependency status
[![Dependency Status](https://david-dm.org/kaelzhang/node-util-inherits.svg)](https://david-dm.org/kaelzhang/node-util-inherits)
-->

# util-inherits

util.inherits with compatibility.

`util-inherits` will try use `Object.setPrototypeOf`, if `Object.setPrototypeOf` is not supported, then `Object.create`, or manipulate prototype.

- Browser friendly.
- Does not rely on node utilities

## Install

```sh
$ npm install util.inherits --save
```

## Usage

```js
const inherits = require('util.inherits')
const {EventEmitter} = require('events')

function MyClass () {
  // code ...
}

inherits(MyClass, EventEmitter)
```

## License

MIT
