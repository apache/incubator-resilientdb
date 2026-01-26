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
import { NewDiffColor } from './color';
import { FlamegraphPalette } from './colorPalette';
import styles from './DiffLegend.module.css';

export type sizeMode = 'small' | 'large';
interface DiffLegendProps {
  palette: FlamegraphPalette;
  showMode: sizeMode;
}

export default function DiffLegend(props: DiffLegendProps) {
  const { palette, showMode } = props;
  const values = decideLegend(showMode);

  const color = NewDiffColor(palette);

  return (
    <div
      data-testid="flamegraph-legend"
      className={`${styles['flamegraph-legend']} ${styles['flamegraph-legend-list']}`}
    >
      {values.map((v) => (
        <div
          key={v}
          className={styles['flamegraph-legend-item']}
          style={{
            backgroundColor: color(v).rgb().toString(),
          }}
        >
          {v > 0 ? '+' : ''}
          {v}%
        </div>
      ))}
    </div>
  );
}

function decideLegend(showMode: sizeMode) {
  switch (showMode) {
    case 'large': {
      return [-100, -80, -60, -40, -20, -10, 0, 10, 20, 40, 60, 80, 100];
    }

    case 'small': {
      return [-100, -40, -20, 0, 20, 40, 100];
    }

    default:
      throw new Error(`Unsupported ${showMode}`);
  }
}
