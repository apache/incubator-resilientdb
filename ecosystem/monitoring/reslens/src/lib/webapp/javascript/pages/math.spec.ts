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

import { calculateMean, calculateStdDeviation } from './math';

describe('math', () => {
  describe('calculateMean, calculateStdDeviation', () => {
    test.each([
      [[1, 2, 3, 4, 5], 1.4142135623730951],
      [[23, 4, 6, 457, 65, 7, 45, 8], 145.13565852332775],
      [[3456, 9876, 12, 0, 0, 99917, 1000000, 234543], 323657.1010328678],
    ])(
      'should calculate correct standart deviation',
      (array, expectedValue) => {
        const mean = calculateMean(array);

        expect(calculateStdDeviation(array, mean)).toBe(expectedValue);
      }
    );
  });
});
