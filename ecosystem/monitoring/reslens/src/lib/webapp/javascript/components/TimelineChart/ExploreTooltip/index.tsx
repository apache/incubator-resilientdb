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

import React, { FC, useMemo } from 'react';
import Color from 'color';
import { getFormatter } from '@pyroscope/flamegraph/src/format/format';
import { Profile } from '@pyroscope/models/src';
import { TimelineTooltip } from '../../TimelineTooltip';
import { TooltipCallbackProps } from '../Tooltip.plugin';

type ExploreTooltipProps = TooltipCallbackProps & {
  profile?: Profile;
};

const ExploreTooltip: FC<ExploreTooltipProps> = ({
  timeLabel,
  values,
  profile,
}) => {
  const numTicks = profile?.flamebearer?.numTicks;
  const sampleRate = profile?.metadata?.sampleRate;
  const units = profile?.metadata?.units;

  const formatter = useMemo(
    () =>
      numTicks &&
      typeof sampleRate === 'number' &&
      units &&
      getFormatter(numTicks, sampleRate, units),
    [numTicks, sampleRate, units]
  );

  const total = useMemo(
    () =>
      values?.length
        ? values?.reduce((acc, current) => acc + (current.closest?.[1] || 0), 0)
        : 0,
    [values]
  );

  const formatValue = (v: number) => {
    if (formatter && typeof sampleRate === 'number') {
      const value = formatter.format(v, sampleRate);
      let percentage = (v / total) * 100;

      if (Number.isNaN(percentage)) {
        percentage = 0;
      }

      return `${value} (${percentage.toFixed(2)}%)`;
    }

    return '0';
  };

  const items = values.map((v) => {
    return {
      label: v.tagName || '',
      color: Color.rgb(v.color),
      value: formatValue(v?.closest?.[1] || 0),
    };
  });

  return <TimelineTooltip timeLabel={timeLabel} items={items} />;
};

export default ExploreTooltip;
