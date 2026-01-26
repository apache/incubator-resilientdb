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

/* eslint-disable import/prefer-default-export */
import groupBy from 'lodash.groupby';
import map from 'lodash.map';
import type { Profile, Trace, TraceSpan } from '@pyroscope/models/src';
import { deltaDiffWrapperReverse } from '../FlameGraph/decode';

interface Span extends TraceSpan {
  children: Span[];
  total: number;
  self: number;
}

export function convertJaegerTraceToProfile(trace: Trace): Profile {
  const resultFlamebearer = {
    numTicks: 0,
    maxSelf: 0,
    names: [] as string[],
    levels: [] as number[][],
  };

  // Step 1: converting spans to a tree

  const spans: Record<string, Span> = {};
  const root: Span = { children: [] } as unknown as Span;
  (trace.spans as Span[]).forEach((span) => {
    span.children = [];
    spans[span.spanID] = span;
  });

  (trace.spans as Span[]).forEach((span) => {
    let node = root;
    if (span.references && span.references.length > 0) {
      node = spans[span.references[0].spanID] || root;
    }

    node.children.push(span);
  });

  // Step 2: group spans with same name

  function groupSpans(span: Span, d: number) {
    (span.children || []).forEach((x) => groupSpans(x, d + 1));

    let childrenDur = 0;
    const groups = groupBy(span.children || [], (x) => x.operationName);
    span.children = map(groups, (group) => {
      const res = group[0];
      for (let i = 1; i < group.length; i += 1) {
        res.duration += group[i].duration;
      }
      childrenDur += res.duration;
      return res;
    });
    span.total = span.duration || childrenDur;
    span.self = Math.max(0, span.total - childrenDur);
  }
  groupSpans(root, 0);

  // Step 3: traversing the tree

  function processNode(span: Span, level: number, offset: number) {
    resultFlamebearer.numTicks ||= span.total;
    resultFlamebearer.levels[level] ||= [];
    resultFlamebearer.levels[level].push(offset);
    resultFlamebearer.levels[level].push(span.total);
    resultFlamebearer.levels[level].push(span.self);
    resultFlamebearer.names.push(
      (span.processID
        ? `${trace.processes[span.processID].serviceName}: `
        : '') + (span.operationName || 'total')
    );
    resultFlamebearer.levels[level].push(resultFlamebearer.names.length - 1);

    (span.children || []).forEach((x) => {
      offset += processNode(x, level + 1, offset);
    });
    return span.total;
  }

  processNode(root, 0, 0);

  resultFlamebearer.levels = deltaDiffWrapperReverse(
    'single',
    resultFlamebearer.levels
  );

  return {
    version: 1,
    flamebearer: resultFlamebearer,
    metadata: {
      format: 'single',
      units: 'trace_samples',
      spyName: 'tracing',
      sampleRate: 1000000,
    },
  };
}
