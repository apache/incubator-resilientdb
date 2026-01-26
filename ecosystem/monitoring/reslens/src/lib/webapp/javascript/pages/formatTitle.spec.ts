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

import { formatTitle } from './formatTitle';
import { brandQuery } from '@webapp/models/query';

describe('format title', () => {
  describe('when both left and right query are falsy', () => {
    it('returns only the page title', () => {
      expect(formatTitle('mypage')).toBe('mypage');
      expect(formatTitle('mypage', brandQuery(''), brandQuery(''))).toBe(
        'mypage'
      );
    });
  });

  describe('when only a single query is set', () => {
    it('sets it correctly', () => {
      expect(formatTitle('mypage', brandQuery('myquery'))).toBe(
        'mypage | myquery'
      );
    });
  });

  describe('when both queries are set', () => {
    it('sets it correctly', () => {
      expect(
        formatTitle('mypage', brandQuery('myquery1'), brandQuery('myquery2'))
      ).toBe('mypage | myquery1 and myquery2');
    });
  });
});
