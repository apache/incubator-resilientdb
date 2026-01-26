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

import { Profile } from '@pyroscope/models/src';

export function deltaDiffWrapperReverse(
  format: Profile['metadata']['format'],
  levels: Profile['flamebearer']['levels']
) {
  const mutableLevels = [...levels];

  function deltaDiff(
    lvls: Profile['flamebearer']['levels'],
    start: number,
    step: number
  ) {
    // eslint-disable-next-line no-restricted-syntax
    for (const level of lvls) {
      let total = 0;
      for (let i = start; i < level.length; i += step) {
        level[i] -= total;
        total += level[i] + level[i + 1];
      }
    }
  }

  if (format === 'double') {
    deltaDiff(mutableLevels, 0, 7);
    deltaDiff(mutableLevels, 3, 7);
  } else {
    deltaDiff(mutableLevels, 0, 4);
  }

  return mutableLevels;
}

export function deltaDiffWrapper(
  format: Profile['metadata']['format'],
  levels: Profile['flamebearer']['levels']
) {
  const mutableLevels = [...levels];

  function deltaDiff(
    lvls: Profile['flamebearer']['levels'],
    start: number,
    step: number
  ) {
    // eslint-disable-next-line no-restricted-syntax
    for (const level of lvls) {
      let prev = 0;
      for (let i = start; i < level.length; i += step) {
        level[i] += prev;
        prev = level[i] + level[i + 1];
      }
    }
  }

  if (format === 'double') {
    deltaDiff(mutableLevels, 0, 7);
    deltaDiff(mutableLevels, 3, 7);
  } else {
    deltaDiff(mutableLevels, 0, 4);
  }

  return mutableLevels;
}

export default function decodeFlamebearer(fb: Profile): Profile {
  // Make a copy since we will modify the undelying data structure
  const copy = JSON.parse(JSON.stringify(fb));

  copy.flamebearer.levels = deltaDiffWrapper(
    copy.metadata.format,
    copy.flamebearer.levels
  );

  return copy;
}
