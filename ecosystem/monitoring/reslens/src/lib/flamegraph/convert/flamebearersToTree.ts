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

import type { Profile } from '@pyroscope/models/src';

export interface TreeNode {
  name: string;
  key: string;
  self: number[];
  total: number[];
  offset?: number;
  children: TreeNode[];
}

export function flamebearersToTree(
  f1: Profile['flamebearer'],
  f2?: Profile['flamebearer']
): TreeNode {
  const globalLookup: { [key: string]: TreeNode } = {};
  const treeSpecificLookup: { [key: string]: TreeNode } = {};
  let root: TreeNode = {
    name: 'total',
    children: [],
    self: [],
    total: [],
    key: '/total',
  };

  (f2 ? [f1, f2] : [f1]).forEach((f, fi) => {
    for (let i = 0; i < f.levels.length; i += 1) {
      for (let j = 0; j < f.levels[i].length; j += 4) {
        const treeSpecificKey: string = [fi, i, j].join('/');
        const name: string = f.names[f.levels[i][j + 3]];
        const offset: number = f.levels[i][j + 0];
        const total: number = f.levels[i][j + 1];
        const self: number = f.levels[i][j + 2];
        let parentGlobalKey = '';
        // searching for parent node
        if (i !== 0) {
          const pi = i - 1;
          const parentLevel = f.levels[pi];
          for (let k = 0; k < parentLevel.length; k += 4) {
            const parentOffset = parentLevel[k + 0];
            const total = parentLevel[k + 1];
            if (offset >= parentOffset && offset < parentOffset + total) {
              const parentTreeSpecificKey = [fi, pi, k].join('/');
              const parentObj = treeSpecificLookup[parentTreeSpecificKey];
              parentGlobalKey = parentObj.key;
              break;
            }
          }
        }

        const globalKey = [parentGlobalKey || '', name].join('/');
        const isNewObject = !globalLookup[globalKey];
        globalLookup[globalKey] ||= {
          name,
          children: [],
          self: [],
          total: [],
          key: globalKey,
        } as TreeNode;
        const obj: TreeNode = globalLookup[globalKey];
        obj.total[fi] ||= 0;
        obj.total[fi] += total;
        obj.self[fi] ||= 0;
        obj.self[fi] += self;
        treeSpecificLookup[treeSpecificKey] = obj;

        if (parentGlobalKey && isNewObject) {
          globalLookup[parentGlobalKey].children.push(obj);
        }
        if (i === 0) {
          root = obj;
        }
      }
    }
  });

  return root;
}
