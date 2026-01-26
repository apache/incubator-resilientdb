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

function deltaDiffWrapper(format: 'single' | 'double', levels: number[][]) {
  const mutable_levels = [...levels];

  function deltaDiff(levels: number[][], start: number, step: number) {
    for (const level of levels) {
      let prev = 0;
      for (let i = start; i < level.length; i += step) {
        level[i] += prev;
        prev = level[i] + level[i + 1];
      }
    }
  }

  if (format === 'double') {
    deltaDiff(mutable_levels, 0, 7);
    deltaDiff(mutable_levels, 3, 7);
  } else {
    deltaDiff(mutable_levels, 0, 4);
  }

  return mutable_levels;
}

export { deltaDiffWrapper };
