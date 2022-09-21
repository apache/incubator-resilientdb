# furi

[![npm](https://img.shields.io/npm/v/furi.svg?maxAge=2592000)](https://www.npmjs.com/package/furi)
[![GitHub repository](https://img.shields.io/badge/Github-demurgos%2Ffuri-blue.svg)](https://github.com/demurgos/furi)
[![Build status](https://img.shields.io/travis/com/demurgos/furi/master.svg?maxAge=2592000)](https://travis-ci.com/demurgos/furi)
[![Codecov](https://codecov.io/gh/demurgos/furi/branch/master/graph/badge.svg)](https://codecov.io/gh/demurgos/furi)

File URI manipulation library.

This library is intended as a toolbox to handle `file://` URIs. It currently
focuses on conversion between file URIs and system dependent paths.

The conversion supports Windows UNC paths, Windows long paths, trailing
separators, special characters and non-ASCII characters.

## Installation

```shell
npm install --save furi
```

## API documentation

See [documentation](https://demurgos.github.io/furi/).

## References

- https://tools.ietf.org/html/rfc3986#section-3.3
- https://url.spec.whatwg.org/
- https://github.com/nodejs/node/blob/deaddd212c499c7ff88d20034753b5f3f00d5153/lib/internal/url.js#L1414
- https://github.com/nodejs/node/blob/master/test/parallel/test-url-pathtofileurl.js
- https://docs.microsoft.com/en-us/windows/desktop/FileIO/naming-a-file

## License

[MIT License](./LICENSE.md)
