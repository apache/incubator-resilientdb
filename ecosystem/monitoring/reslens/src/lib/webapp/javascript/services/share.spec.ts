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

import { Result } from '@webapp/util/fp';
import { ZodError } from 'zod';
import { shareWithFlamegraphDotcom } from './share';
import { setupServer, rest } from './testUtils';
// TODO move this testData to somewhere else
import TestData from './TestData';

describe('Share', () => {
  let server: ReturnType<typeof setupServer> | null;

  afterEach(() => {
    if (server) {
      server.close();
    }
    server = null;
  });

  describe('shareWithFlamegraphDotcom', () => {
    it('works', async () => {
      server = setupServer(
        rest.post(`http://localhost/export`, (req, res, ctx) => {
          return res(ctx.status(200), ctx.json({ url: 'http://myurl.com' }));
        })
      );

      server.listen();
      const res = await shareWithFlamegraphDotcom({
        name: 'myname',
        flamebearer: TestData,
      });

      expect(res).toMatchObject(
        Result.ok({
          url: 'http://myurl.com',
        })
      );
    });

    it('fails if response doesnt contain the key', async () => {
      server = setupServer(
        rest.post(`http://localhost/export`, (req, res, ctx) => {
          return res(ctx.status(200), ctx.json({}));
        })
      );
      server.listen();

      const res = await shareWithFlamegraphDotcom({
        name: 'myname',
        flamebearer: TestData,
      });
      expect(res.error).toBeInstanceOf(ZodError);
    });
  });
});
