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

import React, { ReactNode } from 'react';
import Color from 'color';
import clsx from 'clsx';
import styles from './ChartTitle.module.scss';

const chartTitleKeys = {
  objects: 'Total number of objects in RAM',
  goroutines: 'Total number of goroutines',
  bytes: 'Total amount of RAM',
  samples: 'Total CPU time',
  lock_nanoseconds: 'Total time spent waiting on locks',
  lock_samples: 'Total number of contended locks',
  diff: 'Baseline vs. Comparison Diff',
  trace_samples: 'Total aggregated span duration',
  exceptions: 'Total number of exceptions thrown',
  unknown: '',

  baseline: 'Baseline Flamegraph',
  comparison: 'Comparison Flamegraph',
  selection_included: 'Selection-included Exemplar Flamegraph',
  selection_excluded: 'Selection-excluded Exemplar Flamegraph',
};

interface ChartTitleProps {
  children?: ReactNode;
  className?: string;
  color?: Color;
  icon?: ReactNode;
  postfix?: ReactNode;
  titleKey?: keyof typeof chartTitleKeys;
}

export default function ChartTitle({
  children,
  className,
  color,
  icon,
  postfix,
  titleKey = 'unknown',
}: ChartTitleProps) {
  return (
    <div className={clsx([styles.chartTitle, className])}>
      {(icon || color) && (
        <span
          className={clsx(styles.colorOrIcon, icon && styles.icon)}
          style={
            !icon && color ? { backgroundColor: color.rgb().toString() } : {}
          }
        >
          {icon}
        </span>
      )}
      <p className={styles.title}>{children || chartTitleKeys[titleKey]}</p>
      {postfix}
    </div>
  );
}
