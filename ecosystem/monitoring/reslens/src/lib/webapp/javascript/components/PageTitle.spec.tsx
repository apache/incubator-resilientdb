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

import React from 'react';
import PageTitle, { AppNameContext } from './PageTitle';
import { render, screen, waitFor } from '@testing-library/react';

describe('PageTitle', () => {
  describe("there's no app name in context", () => {
    it('defaults to Pyroscope', async () => {
      render(<PageTitle title={'mypage'} />);

      await waitFor(() => expect(document.title).toEqual('mypage | Pyroscope'));
    });
  });

  describe("there's an app name in context", () => {
    it('suffixes the title with it', async () => {
      render(
        <AppNameContext.Provider value={'myapp'}>
          <PageTitle title={'mypage'} />
        </AppNameContext.Provider>
      );

      await waitFor(() => expect(document.title).toEqual('mypage | myapp'));
    });
  });
});
