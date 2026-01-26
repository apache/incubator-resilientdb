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

// eslint-disable-next-line import/no-relative-packages
import { Maybe } from 'true-myth';
import { version as jsVersion } from '../../../package.json';

export interface BuildInfo {
  goos: string;
  goarch: string;
  goVersion: string;
  version: string;
  time: string;
  gitSHA: string;
  gitDirty: string;
  useEmbeddedAssets: string;
  jsVersion: string;
}

export const buildInfo = function (): BuildInfo {
  // TODO: it may be possible that these fields are not populated
  // but as for now assume it is always present
  const win = window as unknown as { buildInfo: BuildInfo };

  return {
    jsVersion,
    goos: win.buildInfo?.goos,
    goarch: win.buildInfo?.goarch,
    goVersion: win.buildInfo?.goVersion,
    version: win.buildInfo?.version,
    time: win.buildInfo?.time,
    gitSHA: win.buildInfo?.gitSHA,
    gitDirty: win.buildInfo?.gitDirty,
    useEmbeddedAssets: win.buildInfo?.useEmbeddedAssets,
  };
};

interface LatestVersionInfo {
  latest_version: string;
}

// Make it explicit that this field may not be populated
// for cases like standalone
export const latestVersionInfo = function (): Maybe<LatestVersionInfo> {
  const win = window as unknown as { latestVersionInfo: LatestVersionInfo };

  if (!win.latestVersionInfo) {
    return Maybe.nothing();
  }

  return Maybe.of({
    latest_version: win.latestVersionInfo.latest_version,
  });
};
