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
import type { TimelineGroupData } from '@webapp/components/TimelineChart/TimelineChartWrapper';
import { ALL_TAGS } from '@webapp/redux/reducers/continuous';
import classNames from 'classnames/bind';
import styles from './Legend.module.scss';

const cx = classNames.bind(styles);
interface LegendProps {
  groups: TimelineGroupData[];
  handleGroupByTagValueChange: (groupByTagValue: string) => void;
  activeGroup: string;
}

function Legend({
  groups,
  handleGroupByTagValueChange,
  activeGroup,
}: LegendProps) {
  return (
    <div data-testid="legend" className={styles.legend}>
      {groups.map(({ tagName, color }) => {
        const isSelected = tagName === activeGroup;
        return (
          <div
            aria-hidden
            data-testid="legend-item"
            className={cx({
              [styles.tagName]: true,
              [styles.faded]: activeGroup && !isSelected,
            })}
            key={tagName}
            onClick={() =>
              handleGroupByTagValueChange(isSelected ? ALL_TAGS : tagName)
            }
          >
            <span
              data-testid="legend-item-color"
              className={styles.tagColor}
              style={{ backgroundColor: color?.toString() }}
            />
            <span>{tagName}</span>
          </div>
        );
      })}
    </div>
  );
}

export default Legend;
