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

import basename from './baseurl';

function checkSelector(selector: string) {
  if (selector !== 'meta[name="pyroscope-base-url"]') {
    throw new Error('Wrong selector');
  }
}
describe('baseurl', () => {
  describe('no baseURL meta tag set', () => {
    it('returns undefined', () => {
      const got = basename();

      expect(got).toBe(undefined);
    });
  });

  describe('baseURL meta tag set', () => {
    describe('no content', () => {
      beforeEach(() => {
        jest
          .spyOn(document, 'querySelector')
          .mockImplementationOnce((selector) => {
            checkSelector(selector);
            return {} as HTMLMetaElement;
          });
      });
      it('returns undefined', () => {
        const got = basename();

        expect(got).toBe(undefined);
      });
    });

    describe("there's content", () => {
      it('works with a base Path', () => {
        jest
          .spyOn(document, 'querySelector')
          .mockImplementationOnce((selector) => {
            checkSelector(selector);
            return { content: '/pyroscope' } as HTMLMetaElement;
          });

        const got = basename();

        expect(got).toBe('/pyroscope');
      });

      it('works with a full URL', () => {
        jest
          .spyOn(document, 'querySelector')
          .mockImplementationOnce((selector) => {
            checkSelector(selector);
            return {
              content: 'http://localhost:8080/pyroscope',
            } as HTMLMetaElement;
          });

        const got = basename();

        expect(got).toBe('/pyroscope');
      });
    });
  });
});
