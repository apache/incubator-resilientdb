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

import { Profile } from '@pyroscope/models/src';
import Color from 'color';
import { getFormatter } from '@pyroscope/flamegraph/src/format/format';

export interface TableValuesData {
  color?: Color;
  mean: number;
  stdDeviation: number;
  total: number;
  tagName: string;
  totalLabel: string;
  stdDeviationLabel: string;
  meanLabel: string;
}

export function addSpaces(maxLen: number, len: number, label: string) {
  if (!label.includes('.') || !maxLen || !len || len > maxLen) {
    return label;
  }

  return (
    '\xa0'.repeat(maxLen - len).concat(label.split('.')[0]) +
    '.'.concat(label.split('.')[1])
  );
}

export function getIntegerSpaceLengthForString(value?: string) {
  if (!value || !value.includes('.')) {
    return 1;
  }

  return value.split('.')[0].length;
}

export function getTableIntegerSpaceLengthByColumn(data: TableValuesData[]) {
  return data.reduce(
    (acc, current) => {
      const meanIntegerSpaceLength = getIntegerSpaceLengthForString(
        current.meanLabel
      );
      const stdDeviationIntegerSpaceLength = getIntegerSpaceLengthForString(
        current.stdDeviationLabel
      );
      const totalIntegerSpaceLength = getIntegerSpaceLengthForString(
        current.totalLabel
      );

      return {
        ...acc,
        mean:
          meanIntegerSpaceLength > acc.mean ? meanIntegerSpaceLength : acc.mean,
        stdDeviation:
          stdDeviationIntegerSpaceLength > acc.stdDeviation
            ? stdDeviationIntegerSpaceLength
            : acc.stdDeviation,
        total:
          totalIntegerSpaceLength > acc.total
            ? totalIntegerSpaceLength
            : acc.total,
      };
    },
    { mean: 1, stdDeviation: 1, total: 1 }
  );
}

export const formatValue = ({
  value,
  formatter,
  profile,
}: {
  value?: number;
  formatter?: ReturnType<typeof getFormatter>;
  profile?: Profile;
}) => {
  if (!formatter || !profile || typeof value !== 'number') {
    return '0';
  }

  const formatterResult = `${formatter.format(
    value,
    profile.metadata.sampleRate
  )}`;

  if (String(formatterResult).includes('< 0.01')) {
    return formatter.formatPrecise(value, profile.metadata.sampleRate);
  }

  return formatterResult;
};
