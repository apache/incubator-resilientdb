# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
# 
#   http://www.apache.org/licenses/LICENSE-2.0
# 
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.    


import functools
import codecs
from functools import lru_cache


class HDict(dict):
    def __hash__(self):
        return hash(codecs.decode(self['id'], 'hex'))


@lru_cache(maxsize=16384)
def from_dict(func, *args, **kwargs):
    return func(*args, **kwargs)


def memoize_from_dict(func):

    @functools.wraps(func)
    def memoized_func(*args, **kwargs):

        if args[1].get('id', None):
            args = list(args)
            args[1] = HDict(args[1])
            new_args = tuple(args)
            return from_dict(func, *new_args, **kwargs)
        else:
            return func(*args, **kwargs)

    return memoized_func


class ToDictWrapper():
    def __init__(self, tx):
        self.tx = tx

    def __eq__(self, other):
        return self.tx.id == other.tx.id

    def __hash__(self):
        return hash(self.tx.id)


@lru_cache(maxsize=16384)
def to_dict(func, tx_wrapped):
    return func(tx_wrapped.tx)


def memoize_to_dict(func):

    @functools.wraps(func)
    def memoized_func(*args, **kwargs):

        if args[0].id:
            return to_dict(func, ToDictWrapper(args[0]))
        else:
            return func(*args, **kwargs)

    return memoized_func
