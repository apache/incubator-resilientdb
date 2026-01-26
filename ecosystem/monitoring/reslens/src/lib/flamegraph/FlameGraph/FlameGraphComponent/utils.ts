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

/* eslint-disable import/prefer-default-export */
import { doubleFF } from '@pyroscope/models/src';

// not entirely sure where this should fit
function getRatios(
  level: number[],
  j: number,
  leftTicks: number,
  rightTicks: number
) {
  const ff = doubleFF;

  // throw an error
  // since otherwise there's no way to calculate a diff
  if (!leftTicks || !rightTicks) {
    // ideally this should never happen
    // however there must be a race condition caught in CI
    // https://github.com/pyroscope-io/pyroscope/pull/439/checks?check_run_id=3808581168
    console.error(
      "Properties 'rightTicks' and 'leftTicks' are required. Can't calculate ratio."
    );
    return { leftRatio: 0, rightRatio: 0 };
  }

  const leftRatio = ff.getBarTotalLeft(level, j) / leftTicks;
  const rightRatio = ff.getBarTotalRght(level, j) / rightTicks;

  return { leftRatio, rightRatio };
}

export { getRatios };
