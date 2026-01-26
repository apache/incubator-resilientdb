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

import {
  addSpaces,
  getIntegerSpaceLengthForString,
  getTableIntegerSpaceLengthByColumn,
  TableValuesData,
} from './formatTableData';

describe('addSpaces', () => {
  it('works correctly with decimal spaces', () => {
    expect(addSpaces(3, 1, '0.24 minutes')).toBe('  0.24 minutes');
    expect(addSpaces(1, 1, '0.024 minutes')).toBe('0.024 minutes');
  });

  it('handles possible edge cases', () => {
    expect(addSpaces(1, 0, '0.24 minutes')).toBe('0.24 minutes');
    expect(addSpaces(1, 2, '0.24 minutes')).toBe('0.24 minutes');
  });

  it('handles case when no decimal space', () => {
    expect(addSpaces(3, 1, '24 minutes')).toBe('24 minutes');
    expect(addSpaces(3, 1, '1 minute')).toBe('1 minute');
  });
});

describe('getIntegerSpaceLengthForString', () => {
  describe('works correctly w/ all possible params', () => {
    test.each([
      ['0.00046 minutes', 1],
      ['31.90 minutes', 2],
      ['3143.90 minutes', 4],
      ['0 minutes', 1],
      ['', 1],
      [undefined, 1],
      ['10 minutes', 1],
    ])('returns correct value', (params, expectedValue) => {
      expect(getIntegerSpaceLengthForString(params)).toBe(expectedValue);
    });
  });
});

const data = [
  {
    meanLabel: '',
    stdDeviationLabel: '0.05 minutes',
    totalLabel: '10.08 minutes',
  },
  {
    meanLabel: '0.00011 minutes',
    stdDeviationLabel: '1 minute',
    totalLabel: '773.08 minutes',
  },
  {
    meanLabel: '0.11 minutes',
    stdDeviationLabel: '0.05 minutes',
    totalLabel: '10.08 minutes',
  },
];

describe('getTableIntegerSpaceLengthByColumn', () => {
  it('returns correct max integer space length by column', () => {
    const result = getTableIntegerSpaceLengthByColumn(
      data as TableValuesData[]
    );
    expect(result.mean).toBe(1);
    expect(result.stdDeviation).toBe(1);
    expect(result.total).toBe(3);
  });
});
