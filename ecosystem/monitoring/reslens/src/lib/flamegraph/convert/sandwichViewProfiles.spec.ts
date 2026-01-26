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

import { treeToFlamebearer, calleesFlamebearer } from './sandwichViewProfiles';
import { flamebearersToTree } from './flamebearersToTree';

import { tree, singleAppearanceTrees } from './testData';
import { Flamebearer } from '@pyroscope/models/src';

const flamebearersProps = {
  spyName: 'gospy',
  units: 'samples',
  format: 'single',
  numTicks: 400,
  maxSelf: 150,
  sampleRate: 100,
};

describe('Sandwich view profiles', () => {
  describe('when target function has single tree appearance', () => {
    it('return correct flamebearer with 0 callees', () => {
      const f = treeToFlamebearer(tree);

      const resultCalleesFlamebearer = calleesFlamebearer(
        { ...f, ...flamebearersProps } as Flamebearer,
        'name-5-2'
      );

      const treeToMatchOriginalTree = flamebearersToTree(
        resultCalleesFlamebearer
      );

      expect(treeToMatchOriginalTree).toMatchObject(singleAppearanceTrees.zero);
    });
    it('return correct flamebearer with 1 callee', () => {
      const f = treeToFlamebearer(tree);

      const resultCalleesFlamebearer = calleesFlamebearer(
        { ...f, ...flamebearersProps } as Flamebearer,
        'wwwwwww'
      );

      const treeToMatchOriginalTree = flamebearersToTree(
        resultCalleesFlamebearer
      );

      expect(treeToMatchOriginalTree).toMatchObject(singleAppearanceTrees.one);
    });
    it('return correct flamebearer with multiple callees', () => {
      const f = treeToFlamebearer(tree);

      const resultCalleesFlamebearer = calleesFlamebearer(
        { ...f, ...flamebearersProps } as Flamebearer,
        'name-2-2'
      );

      const treeToMatchOriginalTree = flamebearersToTree(
        resultCalleesFlamebearer
      );

      expect(treeToMatchOriginalTree).toMatchObject(
        singleAppearanceTrees.multiple
      );
    });
  });

  // todo(dogfrogfog): add tests for callers flamegraph when remove top lvl empty node
});
