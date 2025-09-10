const inherits = typeof Object.setPrototypeOf === 'function'
  ? function (ctor, superCtor) {
    ctor.super_ = superCtor
    Object.setPrototypeOf(ctor.prototype, superCtor.prototype)
  }

  : typeof Object.create === 'function'
    ? function (ctor, superCtor) {
      ctor.super_ = superCtor;
      ctor.prototype = Object.create(superCtor.prototype, {
        constructor: {
          value: ctor,
          enumerable: false,
          writable: true,
          configurable: true
        }
      })
    }

    : function (ctor, superCtor) {
      ctor.super_ = superCtor
      function F () {}
      F.prototype = superCtor.prototype
      ctor.prototype = new F
      ctor.prototype.constructor = ctor
    }


module.exports = function (ctor, superCtor) {

  if (ctor === undefined || ctor === null) {
    throw new TypeError('The constructor to "inherits" must not be ' +
                        'null or undefined')
  }

  if (superCtor === undefined || superCtor === null) {
    throw new TypeError('The super constructor to "inherits" must not ' +
                        'be null or undefined')
  }

  if (superCtor.prototype === undefined) {
    throw new TypeError('The super constructor to "inherits" must ' +
                        'have a prototype')
  }

  inherits(ctor, superCtor)
}
