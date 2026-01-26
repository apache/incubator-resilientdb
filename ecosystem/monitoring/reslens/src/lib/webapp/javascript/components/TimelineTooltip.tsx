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
import Color from 'color';
import styles from './TimelineTooltip.module.css';

export interface TimelineTooltipProps {
  timeLabel: string;
  items: {
    color?: Color;
    value: string;
    label: string;
  }[];
}

// TimelineTooltip is a generic tooltip to be used with the timeline
// It contains no logic and render items as they are
// Any formatting should be performed by the caller
function TimelineTooltip({ timeLabel, items }: TimelineTooltipProps) {
  return (
    <div>
      <div className={styles.time}>{timeLabel}</div>
      {items.map((a) => (
        <TimelineTooltipItem
          key={`${a.label}-${a.value}`}
          color={a.color}
          label={a.label}
          value={a.value}
        />
      ))}
    </div>
  );
}

function TimelineTooltipItem({
  color,
  label,
  value,
}: TimelineTooltipProps['items'][number]) {
  const ColorDiv = color ? (
    <div
      className={styles.valueColor}
      style={{ backgroundColor: Color.rgb(color).toString() }}
    />
  ) : null;

  return (
    <div className={styles.valueWrapper}>
      {ColorDiv}
      <div>{label}:</div>
      <div className={styles.closest}>{value}</div>
    </div>
  );
}

export { TimelineTooltip };
