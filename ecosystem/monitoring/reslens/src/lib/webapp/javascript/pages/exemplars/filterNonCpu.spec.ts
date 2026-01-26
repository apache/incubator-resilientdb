/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

import { filterNonCPU } from './filterNonCPU';

describe('filterNonCPU', function () {
  describe('an app with cpu suffix is provided', function () {
    it('should be allowed', function () {
      expect(filterNonCPU('myapp.cpu')).toBe(true);
    });
  });

  describe('when an unidentified suffix is available', function () {
    it('should be allowed', function () {
      expect(filterNonCPU('myapp.weirdsuffix')).toBe(true);
    });
  });

  describe('an app with any other supported suffix is provided', function () {
    it.each`
      name                                 | expected
      ${'myapp.alloc_objects'}             | ${false}
      ${'myapp.alloc_space'}               | ${false}
      ${'myapp.goroutines'}                | ${false}
      ${'myapp.inuse_objects'}             | ${false}
      ${'myapp.inuse_space'}               | ${false}
      ${'myapp.mutex_count'}               | ${false}
      ${'myapp.alloc_in_new_tlab_bytes '}  | ${false}
      ${'myapp.alloc_in_new_tlab_objects'} | ${false}
      ${'myapp.lock_count'}                | ${false}
      ${'myapp.lock_duration'}             | ${false}
    `('filterNonCPU($appName) -> $expected', ({ name, expected }) => {
      expect(filterNonCPU(name)).toBe(expected);
    });
  });
});
