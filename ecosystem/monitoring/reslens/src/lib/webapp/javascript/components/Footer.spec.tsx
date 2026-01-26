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
import { render } from '@testing-library/react';
import { Maybe } from 'true-myth';
import * as buildInfo from '../util/buildInfo';
import Footer from './Footer';

const mockDate = new Date('2021-12-21T12:44:01.741Z');

jest.mock('../util/buildInfo.ts');
const actual = jest.requireActual('../util/buildInfo.ts');

const basicBuildInfo = {
  goos: '',
  goarch: '',
  goVersion: '',
  version: '',
  time: '',
  gitDirty: '',
  gitSHA: '',
  jsVersion: '',
  useEmbeddedAssets: '',
};

describe('Footer', function () {
  beforeEach(() => {
    jest.useFakeTimers().setSystemTime(mockDate.getTime());
  });
  afterEach(() => {
    jest.restoreAllMocks();
  });

  describe('trademark', function () {
    beforeEach(() => {
      const buildInfoMock = jest.spyOn(buildInfo, 'buildInfo');
      const latestVersionMock = jest.spyOn(buildInfo, 'latestVersionInfo');

      buildInfoMock.mockImplementation(() => ({ ...basicBuildInfo }));
      latestVersionMock.mockImplementation(() => actual.latestVersionInfo());
    });

    it('shows current year correctly', function () {
      const { queryByText } = render(<Footer />);

      expect(queryByText(/Pyroscope 2020 – 2021/i)).toBeInTheDocument();
    });
  });

  describe('latest version', function () {
    test.each([
      // smaller
      ['0.0.1', '1.0.0', true],
      ['v0.0.1', 'v1.0.0', true],
      ['v9.0.1', 'v10.0.0', true],

      // same version
      ['1.0.0', '1.0.0', false],
      ['v1.0.0', 'v1.0.0', false],
      // current is bigger (bug with the server most likely)
      ['1.0.0', '0.0.1', false],
      ['v1.0.0', 'v0.0.1', false],
      ['v10.0.1', 'v1.0.0', false],
    ])(
      `currVer (%s), latestVer(%s) should show update available? '%s'`,
      (v1, v2, display) => {
        const buildInfoMock = jest.spyOn(buildInfo, 'buildInfo');
        const latestVersionMock = jest.spyOn(buildInfo, 'latestVersionInfo');

        buildInfoMock.mockImplementation(() => ({
          ...basicBuildInfo,
          version: v1,
        }));

        latestVersionMock.mockImplementation(() =>
          Maybe.of({ latest_version: v2 })
        );

        const { queryByText } = render(<Footer />);

        if (display) {
          expect(queryByText(/Newer Version Available/i)).toBeInTheDocument();
        } else {
          expect(
            queryByText(/Newer Version Available/i)
          ).not.toBeInTheDocument();
        }
      }
    );

    it('does not crash when version is not available', () => {
      const buildInfoMock = jest.spyOn(buildInfo, 'buildInfo');
      const latestVersionMock = jest.spyOn(buildInfo, 'latestVersionInfo');

      buildInfoMock.mockImplementation(() => ({ ...basicBuildInfo }));
      latestVersionMock.mockImplementation(() => Maybe.nothing());

      const { queryByRole } = render(<Footer />);
      expect(queryByRole('contentinfo')).toBeInTheDocument();
    });
  });
});
