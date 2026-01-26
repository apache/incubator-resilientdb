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

/* eslint-disable default-case, consistent-return */
import { UnitsType } from '@pyroscope/models/src';
import { SpyNameFirstClassType } from '@pyroscope/models/src/spyName';

export const humanizeSpyname = (n: SpyNameFirstClassType) => {
  switch (n) {
    case 'gospy':
      return 'Go';
    case 'pyspy':
      return 'Python';
    case 'phpspy':
      return 'PHP';
    case 'pyroscope-rs':
      return 'Rust';
    case 'dotnetspy':
      return '.NET';
    case 'ebpfspy':
      return 'eBPF';
    case 'rbspy':
      return 'Ruby';
    case 'nodespy':
      return 'NodeJS';
    case 'javaspy':
      return 'Java';
  }
};

export const humanizeUnits = (u: UnitsType) => {
  switch (u) {
    case 'samples':
      return 'Samples';
    case 'objects':
      return 'Objects';
    case 'goroutines':
      return 'Goroutines';
    case 'bytes':
      return 'Bytes';
    case 'lock_samples':
      return 'Lock Samples';
    case 'lock_nanoseconds':
      return 'Lock Nanoseconds';
    case 'trace_samples':
      return 'Trace Samples';
    case 'exceptions':
      return 'Exceptions';
  }
};

export const isJSONFile = (file: File) =>
  file.name.match(/\.json$/) ||
  file.type === 'application/json' ||
  file.type === 'text/json';
