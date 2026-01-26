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
// eslint-disable-next-line @typescript-eslint/ban-ts-comment
// @ts-nocheck

import type { Profile, Flamebearer } from '@pyroscope/models/src';
import {
  deltaDiffWrapper,
  deltaDiffWrapperReverse,
} from '../FlameGraph/decode';
import { flamebearersToTree } from './flamebearersToTree';

function diffFlamebearer(f1: Flamebearer, f2: Flamebearer): Flamebearer {
  const result: Flamebearer = {
    format: 'double',
    numTicks: f1.numTicks + f2.numTicks,
    leftTicks: f1.numTicks,
    rightTicks: f2.numTicks,
    maxSelf: 100,
    sampleRate: f1.sampleRate,
    names: [],
    levels: [],
    units: f1.units,
    spyName: f1.spyName,
  };

  const tree = flamebearersToTree(f1, f2);
  const processNode = (
    node,
    level: number,
    offsetLeft: number,
    offsetRight: number
  ) => {
    const { name, children, self, total } = node;
    result.names.push(name);
    result.levels[level] ||= [];
    result.maxSelf = Math.max(result.maxSelf, self[0] || 0, self[1] || 0);
    result.levels[level] = result.levels[level].concat([
      offsetLeft,
      total[0] || 0,
      self[0] || 0,
      offsetRight,
      total[1] || 0,
      self[1] || 0,
      result.names.length - 1,
    ]);
    for (let i = 0; i < children.length; i += 1) {
      const [ol, or] = processNode(
        children[i],
        level + 1,
        offsetLeft,
        offsetRight
      );
      offsetLeft += ol;
      offsetRight += or;
    }
    return [total[0] || 0, total[1] || 0];
  };

  processNode(tree, 0, 0, 0);

  return result;
}

export function diffTwoProfiles(p1: Profile, p2: Profile): Profile {
  p1.flamebearer.levels = deltaDiffWrapper('single', p1.flamebearer.levels);
  p2.flamebearer.levels = deltaDiffWrapper('single', p2.flamebearer.levels);
  const resultFlamebearer = diffFlamebearer(p1.flamebearer, p2.flamebearer);
  resultFlamebearer.levels = deltaDiffWrapperReverse(
    'double',
    resultFlamebearer.levels
  );
  const metadata = { ...p1.metadata };
  metadata.format = 'double';
  return {
    version: 1,
    flamebearer: resultFlamebearer,
    metadata,
    leftTicks: p1.flamebearer.numTicks,
    rightTicks: p2.flamebearer.numTicks,
  };
}
