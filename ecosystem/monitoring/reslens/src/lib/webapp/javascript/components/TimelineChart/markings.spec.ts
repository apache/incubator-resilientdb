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

import Color from 'color';
import { markingsFromSelection } from './markings';

// Tests are definitely confusing, but that's due to the nature of the implementation
// TODO: refactor implementatino
describe('markingsFromSelection', () => {
  it('returns nothing when theres no selection', () => {
    expect(markingsFromSelection('single')).toStrictEqual([]);
  });

  const from = 1663000000;
  const to = 1665000000;
  const color = Color('red');

  it('ignores color when selection is single', () => {
    expect(
      markingsFromSelection('single', {
        from: `${from}`,
        to: `${to}`,
        color,
        overlayColor: color,
      })
    ).toStrictEqual([
      {
        color: Color('transparent'),
        xaxis: {
          from: from * 1000,
          to: to * 1000,
        },
      },
      {
        color: Color('transparent'),
        lineWidth: 1,
        xaxis: {
          from: from * 1000,
          to: from * 1000,
        },
      },
      {
        color: Color('transparent'),
        lineWidth: 1,
        xaxis: {
          from: to * 1000,
          to: to * 1000,
        },
      },
    ]);
  });

  it('uses color when selection is double', () => {
    expect(
      markingsFromSelection('double', {
        from: `${from}`,
        to: `${to}`,
        color: color,
        overlayColor: color,
      })
    ).toStrictEqual([
      {
        color: color,
        xaxis: {
          from: from * 1000,
          to: to * 1000,
        },
      },
      {
        color: color,
        lineWidth: 1,
        xaxis: {
          from: from * 1000,
          to: from * 1000,
        },
      },
      {
        color: color,
        lineWidth: 1,
        xaxis: {
          from: to * 1000,
          to: to * 1000,
        },
      },
    ]);
  });
});
