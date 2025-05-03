[![Build Status](https://travis-ci.org/kaelzhang/node-glob-gitignore.svg?branch=master)](https://travis-ci.org/kaelzhang/node-glob-gitignore)
<!-- optional appveyor tst
[![Windows Build Status](https://ci.appveyor.com/api/projects/status/github/kaelzhang/node-glob-gitignore?branch=master&svg=true)](https://ci.appveyor.com/project/kaelzhang/node-glob-gitignore)
-->
<!-- optional npm version
[![NPM version](https://badge.fury.io/js/glob-gitignore.svg)](http://badge.fury.io/js/glob-gitignore)
-->
<!-- optional npm downloads
[![npm module downloads per month](http://img.shields.io/npm/dm/glob-gitignore.svg)](https://www.npmjs.org/package/glob-gitignore)
-->
<!-- optional dependency status
[![Dependency Status](https://david-dm.org/kaelzhang/node-glob-gitignore.svg)](https://david-dm.org/kaelzhang/node-glob-gitignore)
-->

# glob-gitignore

Extends [`glob`](https://www.npmjs.com/package/glob) with support for filtering files according to gitignore rules and exposes an optional Promise API, based on [`node-ignore`](https://www.npmjs.com/package/ignore).

This module is built to solve performance issues, see [Why](#why).

## Install

```sh
$ npm i glob-gitignore --save
```

## Usage

```js
import {
  glob,
  sync,
  hasMagic
} from 'glob-gitignore'

// The usage of glob-gitignore is much the same as `node-glob`,
// and it supports an array of patterns to be matched
glob(['**'], {
  cwd: '/path/to',

  // Except that options.ignore accepts an array of gitignore rules,
  // or a gitignore rule,
  // or an `ignore` instance.
  ignore: '*.bak'
})
// And glob-gitignore returns a promise
.then(files => {
  console.log(files)
})

// A string of pattern is also supported.
glob('**', options)

// To glob things synchronously, use `sync`
const files = sync('**', {ignore: '*.bak'})

hasMagic('a/{b/c,x/y}')  // true
```

## Why

1. The `options.ignore` of `node-glob` does not support gitignore rules.

2. It is better **NOT** to glob things then filter them by gitignore rules. Because by doing this, there will be so much unnecessary harddisk traversing, and cause performance issues, especially if there are tremendous files and directories inside the working directory. For the situation, you'd better to use this module.

`glob-gitignore` does the filtering at the very process of each decending down.

## glob(patterns, options)

Returns a `Promise`

- **patterns** `String|Array.<String>` The pattern or array of patterns to be matched.

And negative patterns (each of which starts with an `!`) are supported, although negative patterns are **NOT** recommended. You'd better to use `options.ignore`.

```js
glob(['*.js', 'a/**', '!a/**/*.png']).then(console.log)
```

- **options** `Object` the [glob options](https://www.npmjs.com/package/glob#options) except for `options.ignore`

### `options.ignore`

Could be a `String`, an array of `String`s, or an instance of [node-`ignore`](https://www.npmjs.com/package/ignore)

Not setting this is kind of silly, since that's the whole purpose of this lib, but it is optional though.

```js
glob('**', {ignore: '*.js'})
glob('**', {ignore: ['*.css', '*.styl']})

import ignore from 'ignore'
glob('**', {
  ignore: ignore().add('*.js')
})
```

## sync(patterns, options)

The synchronous globber, which returns an `Array.<path>`.

## hasMagic(patterns, [options])

This method extends `glob.hasMagic(pattern)` and supports an array of patterns.

Returns

- `true` if there are any special characters in the pattern, or there is any of a pattern in the array has special characters.
- `false` otherwise.

```js
hasMagic('a/{b/c,x/y}')               // true
hasMagic(['a/{b/c,x/y}', 'a'])        // true
hasMagic(['a'])                       // false
```

Note that the options affect the results. If `noext:true` is set in the options object, then `+(a|b)` will not be considered a magic pattern. If the pattern has a brace expansion, like `a/{b/c,x/y}` then that is considered magical, unless `nobrace:true` is set in the options.

## License

MIT
